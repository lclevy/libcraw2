/* 
 * craw2tool: demonstrates libcraw2 library usage
 * 
 * Laurent Clevy (lclevy@free.fr)
 * copyright 2013-2014
 */

// libcraw2 interfaces
#include"craw2.h"
#include"lljpeg.h"
#include"rgb.h"
#include"tiff.h"
#include"yuv.h"

#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>

#define AUTO_TESTS 1

int checkRecompression(struct cr2file *cr2, unsigned long compressedLen, unsigned char *scandata) {
  unsigned long n;  
  unsigned char value;
	
  fseek(cr2->file, cr2->jpeg.scanDataOffset, SEEK_SET);
  for(n=0; n<compressedLen && n<cr2->jpeg.rawBufferLen; n++)
    if ((value = fgetc(cr2->file)) != scandata[n]) {
      printf("n=%ld scandata=0x%x ftell=0x%lx filebyte=0x%x\n", n, scandata[n], fgetc(cr2->file), value);
    }
  n = (cr2->stripByteCounts-(cr2->jpeg.scanDataOffset-cr2->stripOffsets))-compressedLen;     
  
  return (fgetc(cr2->file)==0xff && fgetc(cr2->file)==0xd9);
}

// experimental: to include a text (from a PPM) before reslicing, recompression and patching
int insertPBMat(long x, long y, long width, char *name, unsigned short *img, struct cr2file *cr2) {
  FILE *in;
  long h, l;
  unsigned long c, w, base;
  char buf[10];
  unsigned short val;
  in=fopen(name, "r");
  if (!in) {
    fprintf(stderr,"insertPBMat: fopen\n");
    return -1;
  }  
  fgets(buf, 10, in);
  if (strcmp(buf,"P2\n")!=0){
    fprintf(stderr,"insertPBMat: not P1 format\n");
    return -1;
  }  
  fscanf(in, "%ld", &w);	
  fscanf(in, "%ld", &h);	

  base = y*width+x;
  for(l=0; l<h; l++) {
    for(c=0; c<w; c++) {
      if ( (base+c) >=cr2->unslicedLen ) // out of image borders
        return -1;
      fscanf( in, "%hu", &val ); 
      if ( val > (1<<(cr2->jpeg.bits-1)) ) // out of bits depth
        img[base+c] = (1<<(cr2->jpeg.bits-1));
      else
        img[base+c] = val;
              
    }  
    base += (width);	
  }
  fclose(in);
  return 0;
}

void usage() {
  printf("usage: craw2tool [options] file.cr2\n");
  printf("-h = help\n");
  printf("-v = verbose (default 0, max 4)\n");
  printf("-i = Information. Summary of CR2 properties in one line.\n");
  printf("-t = TIFF tags. Display TIFF tags list.\n");
  printf("-l Line_number = verbose line. Display lossless jpeg low-level operations for a given line number. -1 means last line (like python array indexes).\n");
  printf("-m = Model data. Display CR2 and images properties in csv format\n");
  printf("-V = display libcraw2 version.\n");
  printf("-T = write auto tests intermediate files.\n");
  //printf("-s = display image color statistics\n");
  printf("-g context,tag_id,access = Grep a TIFF tag value. Context=0,1,2,3,e or m. Tag-id in decimal. Access=v(alue),a(array) or b(inary)\n");
  printf("-p patch_image_filename (experimental) = patch original image data with a monochrome image\n");
  printf("-b top,left,bottom,right = Force image borders. Limits are inclusive, bottom and right pixels are included.\n"); 
  printf("-o output_filename = export filename\n");
  printf("-e export_formats_list (comma separated). Could be 'uncomp', 'unsliced', 'rggb', 'yuv', 'yuv16i', 'yuv16p'= export image data steps\n");
  printf("-r = recompress. Decompress, unslice, reslice, recompress and patch the lossless jpeg image\n");
  printf("                 of the original CR2 file by default as 'craw_patched.cr2', which must be identical to original.\n");
}

/*
 * size in short
 */
int exportFile(char *outfile, char *suffix, void* buf, unsigned long size, int verbose) {
  FILE* out;
  char tmpfilename[256];
  if (!buf) {
    fprintf(stderr,"exportFile: buf==0");
    return -1;
  }  
  if (size<0)
    return -1;  
  if (!outfile) 
    strcpy(tmpfilename, "craw2");
  else
    strcpy(tmpfilename, outfile);
  strcat(tmpfilename, suffix);

  out=fopen(tmpfilename, "wb");
  if (out) {
    if (verbose)
      printf("exporting %s\n", tmpfilename);
    fwrite(buf, sizeof(unsigned short), size, out); // endianness !!!
    fclose(out);
    return 0;
  }
  else 
    return -1;
    
  return 0;
}

/*
 * extract TIFF tag values
 *  tagContext: e=exif, m=makernote, 0=ifd#0, 1=ifd#1...
 *  tagId = TAG Id
 *  tagAccess: v=value (direct), a=array (tag->type is used, indirect), b=binary (byte dump, indirect)
 *   "indirect" means tag->value is used as pointer to content. tag content length is supposed getTagTypeLen(tag->type)*tag->len
 */
int grepTag(struct cr2file *cr2Data, char tagContext, unsigned short tagId, char tagAccess) {
  int typeLen;
  struct tiff_tag *tag;
  unsigned char *tagvalBuf, *buf;
  unsigned short *tagvalBuf16;
  unsigned long *tagvalBuf32;
  int notFound;
  unsigned i;
  int contextVal = -1;
  
  // convert context char into parent Tag ID (exif or makernote) or IFD number
  switch(tagContext){
  case 'e':
    contextVal = TIFFTAG_VAL_EXIF;
    break;
  case 'm':
    contextVal = TIFFTAG_VAL_MAKERNOTE;
    break;
  default:
    if ( isdigit(tagContext) )
      contextVal = tagContext-'0';
    else {
      printf("unknown context: %c\n", tagContext );
      return -1;
    }    
  }  

  notFound = 1;
  tag = cr2Data->tagList;
  while(tag!=0 && notFound) { // tagContext='0' for IFD#0, 'e' for exif, 'm' for makernote
    if ( contextVal==tag->context && tagId==tag->id ) {
      notFound = 0;
    }  
    else
      tag = tag->next;
  }
  if (!notFound) {
    if (tagAccess=='a' || tagAccess=='b') {
      if ( cRaw2GetTagValue( tag, &tagvalBuf, cr2Data->file)<0 )
        return -1;
    }
    buf = tagvalBuf; // because tagvalBuf will be incremented, so we will use "free(buf)" 
    tagvalBuf16 = (unsigned short *)tagvalBuf;  
    tagvalBuf32 = (unsigned long *)tagvalBuf;  

    // unlike with -t option, no guess is made, here we follow the tagAccess parameter given   
    switch(tagAccess) { // v=value, a=array, b=binary
    case 'v':
      printf("%lu\n", tag->val);
      break;
    case 'a':
      for(i=0; i<tag->len; i++)    
        switch(tag->type) {
        case TIFFTAG_TYPE_UCHAR:
        case TIFFTAG_TYPE_STRING:
          printf("%c", *tagvalBuf );
          tagvalBuf++;
          break;  
        case TIFFTAG_TYPE_USHORT:
          printf("%hu ", *tagvalBuf16 );
          tagvalBuf16++;
          break;  
        case TIFFTAG_TYPE_ULONG:
          printf("%lu ", *tagvalBuf32 );
          tagvalBuf32++;
          break;  
        case TIFFTAG_TYPE_URATIONAL:
          printf("%lu ", *tagvalBuf32 );
          printf("/ %lu ", *(tagvalBuf32+1) );
          tagvalBuf32+=2;
          break;  
        case TIFFTAG_TYPE_RATIONAL:
          printf("%ld ", (long)*tagvalBuf32 );
          printf("/ %ld ", (long)*(tagvalBuf32+1) );
          tagvalBuf32+=2;
          break;  
        }
        putchar('\n');  
        free(buf);
      break;
    case 'b':
      typeLen = getTagTypeLen( tag->type ); 
      fwrite(tagvalBuf, sizeof(char), tag->len * typeLen, stdout);
      free(buf);
      break;      
    default:
      printf("grep: unknow output format %c\n", tagAccess);    
    }//switch
  }// if notFound
  return 0;  
}

/*
 * print in one line CR2/jpeg/borders properties
 * used by cr2_database.sh
 * used with -m option (model)
 */
void printCr2Properties(struct cr2file *cr2) {
  int i;
  printf("0x%08lx, %s, %d, %d, %d, %d, ", cr2->modelId, cr2->model, cr2->jpeg.bits, cr2->jpeg.wide, cr2->jpeg.high, cr2->jpeg.ncomp); 
  printf("%d, %d, ", cr2->jpeg.hsf, cr2->jpeg.vsf );
  for(i=0; i<CR2_SLICES_LEN; i++) 
    printf("%d, ", cr2->cr2Slices[i]);
  printf("%d, %d, ", cr2->sensorWidth, cr2->sensorHeight );
  printf("%d, %d, %d, %d, ", cr2->sensorLeftBorder, cr2->sensorTopBorder, cr2->sensorRightBorder, cr2->sensorBottomBorder);
  printf("%d, %d, %d, %d, ", cr2->blackMaskLeftBorder, cr2->blackMaskTopBorder, cr2->blackMaskRightBorder, cr2->blackMaskBottomBorder);
  printf("%d, %d, %d\n", cr2->exifImageWidth, cr2->exifImageHeight, cr2->vShift);
}


int main(int argc, char* argv[])
{
  struct cr2file cr2;
  int i;
  char *filename=0, *formatslist=0;
  //char *proclist=0;
  unsigned short corners[4];
  char *outfile=0;
  char tmpfilename[256]="";
  int modelFlag=0, test_flag=0;
  
#ifdef OUTPUT_PIXEL_BIN 
  FILE *out2;
#endif
  unsigned char *scandata;
  unsigned long compressedLen;
  unsigned short *resliced;
  int info=0, recomp=0, tiff_tags=0, imageStats=0, uncompWidth=0, csv=0, csv_follow=0, csv_max=40, dualPixel=0;
  struct cr2image image;
  int quiet=0;
  int pixelVerbose = -10000; // disabled

  char *patchImage=0;
  
  char tagContext=0;
  unsigned short tagId;
  char tagAccess;
  
  // init custom border values 
  for(i=0; i<4; i++) // if corners[2]!=0 -b option has been used 
    corners[i] = 0;
	
  if (argc<2) {
    usage();	
    exit(1);
  }
  // parses command line args (far from bullet proof!)
  i=0;
  while (i<argc) {
    if (argv[i][0]=='-') {
      switch(argv[i][1]) {
      case 'h': // help
        usage();	
        exit(1);
        break;
      case 'v': // verbose level
        i++;
        if (i>=argc) {
          usage();	
          exit(1);
        }
        setVerboseLevel( atoi(argv[i]) );  
        break;
      case 'r': // recompress
        recomp = 1;  
        break;
      case 'V': // version
        printf("libcraw2 v%s\n", CRAW2_VERSION_STR);
        break;
      case 'q': // quiet
        quiet = 1;  
        break;
      case 'p': // patch
        i++;
        patchImage = strdup(argv[i]);  
        break;
      case 'b': // image borders
        i++;
        sscanf(argv[i], "%hu,%hu,%hu,%hu", corners, corners+1, corners+2, corners+3); // "top,left,bottom,right" stored in corners[top,left,bottom,right]
        break;
      case 'g': // grep TIFF tag value
        i++;
        sscanf(argv[i], "%c,%hu,%c", &tagContext, &tagId, &tagAccess); 
        break;
      case 'c': // TIFF tags as csv table
        csv = 1;
        i++;
        sscanf(argv[i], "%d", &csv_max); 
        break;
      case 't': // tiff tags
        tiff_tags = 1;  
        break;
      case 'T': // test
        test_flag = 1;  
        break;        
      case 'l': // pixel verbose 
        i++;
        pixelVerbose = atoi(argv[i]);  
        break;
      case 'w': // change default image width (first slices width) for uncompressed output 
        i++;
        uncompWidth = atoi(argv[i]);  
        break;      
      case 'i': // image info
        info = 1;  
        break;
      case 'm': // model info
        modelFlag = 1;  
        break;
      case 's': // image stats
        imageStats = 1;  
        break;
      case 'j': // jpeg info
        displayJpegHeader( 1 );  
        break;
      case 'd': // dual
        dualPixel = 1;
        break;
      case 'o': // output filename
        i++;  
        outfile = argv[i];
        break;
      case 'e': // export formats list
        i++;  
        formatslist = argv[i];
        break;
      default:
        printf("unknown option %s\n", argv[i]);
        usage();
      } // switch
    }
    else {
      if (i==(argc-1))
        filename = argv[i];
    }
    i++;   
  } // while

  if (filename) {
    if (cRaw2Open(&cr2, filename )<0) 
      exit(1);
	}
	else {
	  usage();
    exit(1);
  }
	
  // retrieve important TIFF tags for RAW processing
  if ( cRaw2GetTags( &cr2)<0 ) // cr2.stripOffsetsB!=0 if dualPixel
	  exit(1);
  
  // display TIFF tags already retrieved 
  if (tiff_tags)
	  displayTagList(cr2.tagList, cr2.file);
 	
  if (csv)
    if (csv_max>0)
      displayTagListCsv(cr2.tagList, cr2.file, 1, csv_max); // follow
    else
      displayTagListCsv(cr2.tagList, cr2.file, 0, csv_max); // no follow

    
  if (tagContext!=0)
    grepTag(&cr2, tagContext, tagId, tagAccess);
  
  if (jpegParseHeaders(cr2.file, cr2.stripOffsets, &(cr2.jpeg)) < 0) {
    fprintf(stderr, "cr2.jpeg: jpegParseHeaders error\n");
    exit(1);
  }
  if (cr2.stripOffsetsB) { // dualPixel
    if (jpegParseHeaders( cr2.file, cr2.stripOffsetsB, &(cr2.jpegB) )<0) {
		fprintf(stderr, "cr2.jpegB: jpegParseHeaders error\n");
		exit(1);
	}
  }

  // enable option to display jpeg decompression details
  if (pixelVerbose<0)
    displayLineJpeg ( cr2.jpeg.high + pixelVerbose ); // -1 means "last line", like in Python
  else
    displayLineJpeg( pixelVerbose );

  if (info && !quiet)
    puts( cRaw2Info(&cr2) );
    
  // decompress from file to jpeg.rawBuffer
  jpegDecompress( &(cr2.jpeg), cr2.file, cr2.stripOffsets, cr2.stripByteCounts );
  if (cr2.stripOffsetsB)   
    jpegDecompress( &(cr2.jpegB), cr2.file, cr2.stripOffsetsB, cr2.stripByteCountsB );
  if (uncompWidth==0) {
    if (cr2.cr2Slices[0]>0)
      uncompWidth = cr2.cr2Slices[1];  
    else
      uncompWidth = (cr2.jpeg.wide * cr2.jpeg.hvCount) / cr2.jpeg.hsf;  // 20D and 1D Mark 2 which have no 0xc640 tag 
  }    
  if (formatslist && strstr(formatslist,"uncomp")!=0) {
    export16BitsBufferAsTiff( "uncompressed.tiff", cr2.jpeg.rawBuffer, cr2.jpeg.rawBufferLen/uncompWidth, uncompWidth ); 
    if (dualPixel && cr2.stripOffsetsB)
      export16BitsBufferAsTiff( "uncompressedB.tiff", cr2.jpegB.rawBuffer, cr2.jpegB.rawBufferLen/uncompWidth, uncompWidth ); 
  }
  // unslice from jpeg.rawBuffer to unsliced
  cRaw2Unslice( &cr2 );

  if (formatslist && strstr(formatslist,"unsliced")!=0) {
    export16BitsBufferAsTiff( "unsliced.tiff", cr2.unsliced, cr2.unslicedLen/cr2.sensorWidth, cr2.sensorWidth ); 
    if ( hasDualPixel(&cr2) )
      export16BitsBufferAsTiff( "unslicedB.tiff", cr2.unslicedB, cr2.unslicedLen/cr2.sensorWidth, cr2.sensorWidth ); 
  }  
  if (test_flag)     
    exportFile(outfile, "_unsliced.bin", cr2.unsliced, cr2.unslicedLen, 0);  

  // autotest: reslicing and recompression of original data must be identical to original
  if (recomp || patchImage) {
  
    if (patchImage) { // patch RAW data with PPM image which filename is patchImage
      insertPBMat(300, 300, cr2.jpeg.wide*cr2.jpeg.ncomp, patchImage, cr2.unsliced, &cr2);
      free(patchImage);
    }
    
    resliced=malloc(sizeof(short) * cr2.jpeg.rawBufferLen);
    if (!resliced) {
      fprintf(stderr, "reslice malloc error");
      exit(1);
    }
    // slice cr2.unsliced[] data into resliced[]
    cRaw2Slice( &cr2, resliced );

    scandata=malloc( cr2.jpeg.rawBufferLen *2);
    if (!scandata) {
      fprintf(stderr, "scandata malloc error");
      exit(1);
    }
    // compress resliced[] into scandata[]
    jpegCompress(&(cr2.jpeg), resliced, scandata, &compressedLen, 0, 0);
	  free(resliced);
		
    if (!patchImage) // patched data can not be identical...
      if ( checkRecompression(&cr2, compressedLen, scandata) )
        printf("re-compression OK\n");

	  // by default, outputs "craw2_patched.cr2"
    if (!outfile)
      strcpy(tmpfilename, "craw2");
    else
      strcpy(tmpfilename, outfile);
    strcat(tmpfilename, "_patched.cr2");

    cRaw2PatchAs(tmpfilename, &cr2, scandata, compressedLen, 0);

    free(scandata);
  }// recomp!=0 || patchImage

  if (cr2.jpeg.hsf==1) { // for RAW only
    int rc;
    if (corners[2]!=0) // -b option has been used
      rc = cRaw2FindImageBorders(&cr2, corners);
    else	
      rc = cRaw2FindImageBorders(&cr2, 0); // find automatically borders
  
    if (rc!=0)
      exit(-1);
  
    if (modelFlag) // image.vShift is computed by cRaw2FindImageBorders()
      printCr2Properties(&cr2);
    
    cr2ImageNew(&image);
    if (corners[2]!=0) // -b option has been used
      cr2ImageCreate(&cr2, &image, corners);
    else  
      cr2ImageCreate(&cr2, &image, 0);
        
    if (imageStats)
      computeImageStats(&image);

    if (formatslist && strstr(formatslist,"rggb")!=0) {
      exportRggbTiff(&cr2, "rggb.tiff" ); // export 16bits RAW greyscale, like dcraw -T -4 -D
    }
    cr2ImageFree(&image);

  } // RAW
  else { // YUV
    //struct cr2file cr2b;
    
    if (modelFlag) 
      printCr2Properties(&cr2);

    if (imageStats)
      yuvComputeImageStats(&cr2);

    if (!cr2.yuvMask)
      yuvMaskCreate(&cr2); // create a mask array where 1 means "existing value", 0 "missing value"
    //exportFile(outfile, "_mask.bin", cr2.yuvMask, (cr2.jpeg.wide*cr2.jpeg.ncomp*cr2.jpeg.high)/2, 1); // size in short

    if (formatslist && strstr(formatslist,"yuv16u")!=0)
      exportYuvTiff(&cr2, "yuv16u.tiff", 0, 0); // not interpolated
    if (formatslist && strstr(formatslist,"yuv16i")!=0)
      exportYuvTiff(&cr2, "yuv16i.tiff", 1, 0); // interpolate before saving, unsliced is modified
    if (test_flag) {    
      exportFile(outfile, "_yuv.bin", cr2.unsliced, cr2.unslicedLen, 0);
    }
    if (formatslist && strstr(formatslist,"yuv16p")!=0)
      exportYuvTiff(&cr2, "yuv16p.tiff", 0, 1); // packed, unsliced not modified, but interpolated data are removed by packing 

    /*
    yuvTiffOpen(&cr2b, "yuv16p.tiff");
    
    printf("cr2.jpeg.hsf=%d\n", cr2b.jpeg.hsf);
    printf("cr2.jpeg.vsf=%d\n", cr2b.jpeg.vsf);
    printf("cr2.jpeg.wide=%d\n", cr2b.jpeg.wide);
    printf("cr2.jpeg.high=%d\n", cr2b.jpeg.high);
    printf("cr2.jpeg.bits=%d\n", cr2b.jpeg.bits);
    printf("cr2.modelId=0x%lx\n", cr2b.modelId);
    printf("cr2.firmware=%s\n", cr2b.firmware);
    printf("cr2.unsliced=%p\n", cr2b.unsliced);
    printf("cr2.unslicedLen=%ld\n", cr2b.unslicedLen);
    printf("cr2.wbRggb=");
    for(i=0; i<4; i++)
      printf("%d ",cr2b.wbRggb[i]);
    printf("\ncr2.srawRggb=");
      for(i=0; i<4; i++)
      printf("%d ",cr2b.srawRggb[i]);
    printf("\n");
    exportFile(outfile, "_yuv2.bin", 0, cr2b.unsliced, cr2b.unslicedLen, 0);
    yuvInterpolate(&cr2b); 
    exportFile(outfile, "_yuvi2.bin", 0, cr2b.unsliced, cr2b.unslicedLen, 0);
    // diff craw2_yuvi.bin craw2_yuvi2.bin
    */
    cr2ImageNew(&image);
    image.isRGGB = 0; // YCbCr and not RGGB
    yuvInterpolate(&cr2);
    if (test_flag)      
      exportFile(outfile, "_yuvi.bin", cr2.unsliced, cr2.unslicedLen, 0);
     
    yuv2rgb(&cr2, &image); // conversion to RGB and WB correction (dcraw method)
    if (test_flag)        
      exportFile(outfile, "_yuv2rgb.bin", image.pixels, image.width *image.height*3, 0);
    
/*
    cr2ImageNew(&image2);
    image2.isRGGB = 0; // YCbCr and not RGGB
    yuv2rgb(&cr2b, &image2); // conversion to RGB and WB correction (dcraw method)
    exportFile(outfile, "_yuv2rgb2.bin", 0, image2.pixels, image2.width *image2.height*3, 0);
    */
    if (formatslist && strstr(formatslist,"yuv2rgb")!=0) // used by test_tiff.sh
      exportRgbTiff(&image, "yuv2rgb.tiff", 0);
    /*if (formatslist && strstr(formatslist,"yuv2rgb")!=0)
      exportFile(outfile, "_yuv2rgb.bin", image.pixels, image.width *image.height*3, 1);*/
    //  exportRgbTiff(&image2, "yuv2rgb2.tiff");
    //cr2ImageFree(&image2);
    //  cRaw2Close( &cr2b );  

    cr2ImageFree(&image);
    
  } // end of YUV

  cRaw2Close( &cr2 );  
	
  return(0);
}

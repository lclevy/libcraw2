// craw2.c

#include"tiff.h"
#include"lljpeg.h"
#include"craw2.h"
#include"rgb.h" // for cr2->filter
#include"yuv.h"

int cr2Verbose=0;

void setVerboseLevel(int lev) {
  cr2Verbose = lev;
}

uint16_t fiGet16(FILE *f) {
	uint8_t a = fgetc(f);
	uint8_t b = fgetc(f);
	return a  | (b << 8);
}
uint32_t fiGet32(FILE *f) {
	uint16_t a = fiGet16(f);
	uint16_t b = fiGet16(f);
	return a | (b << 16);
}
void fiPut16(uint16_t c, FILE *f) {
	fputc(c & 0xff, f);
	fputc((c & 0xff00) >> 8, f);
}
void fiPut32(uint32_t c, FILE *f) {
	fiPut16(c & 0xffff, f);
	fiPut16((c & 0xffff0000) >> 16, f);
}

void cRaw2New(struct cr2file *cr2) {
  int i;
  
  jpegNew( &(cr2->jpeg) );
  jpegNew( &(cr2->jpegB) ); // no cost to always init it 
  cr2->file = 0;
  cr2->model = 0;
  cr2->stripOffsets = 0;
  cr2->stripOffsetsB = 0;
  cr2->unsliced = 0; // after decompression and unslicing
  cr2->unslicedLen = 0;
  cr2->tagList=0;
  cr2->interpolated = 0;
  cr2->height = cr2->width = 0; // if 0 findBorders() is called at beginning of createImage()
  cr2->vShift = 0;
  cr2->yuvMask = 0;
  for(i=0; i<3; i++)
    cr2->cr2Slices[i]=0;
  for(i=0; i<3; i++)
    cr2->cr2SlicesB[i]=0;
}

int cRaw2Open(struct cr2file *cr2, char *name) {
	
  if (!cr2) {
    fprintf(stderr, "cr2 is null");
    return -1;
  }
  cRaw2New( cr2 );
  
  cr2->file = (FILE*)fopen(name, "rb");
  if (!cr2->file) {
    fprintf(stderr, "file error %s\n", name);
    return -1;
  }
  return 0;
}

void cRaw2Close(struct cr2file *cr2) {
  if (cr2) {
    fclose(cr2->file);
    cr2->file = NULL;
  }
  if (cr2->tagList);
    freeTagList(cr2->tagList);
  if (cr2->unsliced)  
    free(cr2->unsliced); 
  if (cr2->model)
    free(cr2->model);  
  if (cr2->yuvMask)
    free(cr2->yuvMask);  
  jpegFree( &(cr2->jpeg) );	
  if (cr2->stripOffsetsB)
    jpegFree( &(cr2->jpegB) );
}

int hasDualPixel(struct cr2file *cr2) {
  return (cr2->stripOffsetsB)!=0;
}


/*
 * tested for all models including 7D Mark II
 */
void cRaw2GetWbValues(struct cr2file *cr2, struct tiff_tag *tag )
{
  int i, j;
  int version;
  
  fseek(cr2->file, tag->val, SEEK_SET);
  version = fiGet16(cr2->file);
	if (cr2Verbose>1) 
    printf("\n0x4001 version=%d\n",version);
  
  fseek(cr2->file, tag->val, SEEK_SET);
  switch(tag->len) {
  case 582: // for 20D and 350D, "ColorBalance1" WB_RGGBLevelsAsShot
    i = 25;
    break;
  case 653: // for 1D Mark II and 1Ds Mark II, "ColorBalance2" WB_RGGBLevelsAsShot
    i = 34;
    break;
  case 5120: // for G10, "ColorData5"
    i = 71;
    break;
  default:
    i = 63; // See "ColorBalance3", "ColorBalance4", ColorData6   WB_RGGBLevelsAsShot tags 
  }
	fseek(cr2->file, i*sizeof(short), SEEK_CUR);

  for(j=0; j<4; j++)
    cr2->wbRggb[j] = fiGet16(cr2->file);
		
	if (tag->len==1312 || tag->len==1316 || tag->len==1313) // ColorData7, EOS 1DX (1316), 5DmkIII (1312), 70D (1313), 6D (1313)
    fseek(cr2->file, 56*sizeof(short), SEEK_CUR);
  else  	
    fseek(cr2->file, 11*sizeof(short), SEEK_CUR);
		
  for(j=0; j<4; j++)
    cr2->srawRggb[j] = fiGet16(cr2->file);
    
  if (cr2Verbose>1) {
    printf("wbRggb ");
    for(i=0; i<4; i++)
      printf("%d ",cr2->wbRggb[i]);
    printf("\nsrawRggb ");
    for(i=0; i<4; i++)
      printf("%d ",cr2->srawRggb[i]);
    putchar('\n');  
  }	
}

/*
 * 	if not NULL, corners = [ sensorTopBorder, sensorLeftBorder, sensorBottomBorder, sensorRightBorder	]
 */
int cRaw2FindImageBorders(struct cr2file *cr2, unsigned short *corners) {
  unsigned long rowWidth;
  unsigned short line, column, l, c, color, pixel;
  int i;
  double sums[NB_COLORS];
  
  cr2->vShift = 0;
  for(i=0; i<NB_COLORS; i++)
    sums[i] = 0.0;
  //rowWidth = cr2->jpeg.wide * cr2->jpeg.ncomp;
  rowWidth = cr2->sensorWidth;
  
  if (corners) { // we force image borders (for dcraw compatibility)
    if (corners[0]<0 || corners[1]<0 || corners[3]>=rowWidth || corners[2]>=cr2->sensorHeight) {
      fprintf(stderr,"error: image corners out of image ");
      return -1;
    }
    if (corners[2]<corners[0]) {
      fprintf(stderr,"error: image corners left>=right\n");
      return -1;
    }
    if ( corners[3]<corners[1]) {
      fprintf(stderr,"error: image corners top>=bottom\n");
      return -1;
    }
    cr2->width = corners[3]-corners[1]+1; // sensorRightBorder-sensorLeftBorder+1
    cr2->height = corners[2]-corners[0]+1; // sensorBottomBorder-sensorTopBorder+1
    cr2->tBorder = corners[0];
    cr2->lBorder = corners[1];
    cr2->bBorder = corners[2];
    cr2->rBorder = corners[3];
    
  } // corners
  else { // corners==0
  
	// we use dimension from exif fields (exifImageWidth, exifImageHeight)
	// and position from sensor_info (sensorTopBorder, sensorBottomBorder, sensorLeftBorder, sensorRightBorder)
    cr2->width = cr2->exifImageWidth;
    cr2->height = cr2->exifImageHeight;
    cr2->tBorder = cr2->sensorTopBorder;
    cr2->bBorder = cr2->sensorBottomBorder;
    cr2->lBorder = cr2->sensorLeftBorder;
    cr2->rBorder = cr2->sensorRightBorder;
  }
  
  if ( (cr2->width)&1 || (cr2->height)&1 ) { // cr2->height might be odd !!! (dcraw does)
    fprintf(stderr,"error image dimension width=%d height=%d\n", cr2->width, cr2->height);
    return -1;
  }

  /* but sometimes (with Canon dimensions from exif and sensor_info), the first line is G2/B instead of R/G1
   * so we measure the difference between candidates R/B and G1/G2, ABS(G1-G2) must be << ABS(R-B)
   * if not, we shift the image one line down
   * WE TRUST 'forced' horizontal dimensions!!! no "hShift"
   */	 
	for(line = cr2->tBorder, l=0; line <= cr2->bBorder; line++, l++) {
	  for(column = cr2->lBorder, c=0; column <= cr2->rBorder; column++, c++) {
        color = FILTER_COLOR(l, c); // hypothesis: even lines are R/G1, odd are G2/B. Column are always OK
        pixel = cr2->unsliced[ line*rowWidth + column ];
        sums[color]+= pixel;
      }
    }
  // ABS(G1-G2) must be << ABS(R-B)
	if ( ABS(sums[0]- sums[3]) < ABS(sums[1]- sums[2]) )  // test if G2-B on first line, instead of R-G1
		cr2->vShift = 1;

	if (cr2Verbose>0)
	  printf("\nImageHeight=%d [%d-%d], ImageWidth=%d [%d-%d]. vshift=%d\n", cr2->height, cr2->tBorder, cr2->bBorder, 
	    cr2->width,  cr2->lBorder, cr2->rBorder, cr2->vShift );

  return 0;       
}

/*
 * extract stripOffsets, stripByteCounts and cr2Slices from RAW IFD (#3 or #4)
 */
int analyseRawIFD(unsigned long ifdOffset, FILE* file, unsigned long* stripOffsets, unsigned long* stripByteCounts, unsigned short* cr2Slices ) {
  unsigned short n, i, j;
  unsigned short tagId, tagType;
  unsigned long saveFileOffset, tagNb, tagValue;
  
  fseek(file, ifdOffset, SEEK_SET);
  n = fiGet16(file); // number of tags in the RAW IFD
  for(i=0; i<n; i++) {
    tagId = fiGet16(file);
    tagType = fiGet16(file);
    tagNb = fiGet32(file);
    tagValue = fiGet32(file);
    /*if (cr2Verbose>1)
      printf("0x%04x %2d 0x%lx 0x%lx\n", tagId, tagType, tagNb, tagValue);*/
    switch(tagId) {
    case IFD3_STRIPOFFSETS:
      *stripOffsets = tagValue;
      break;
    case IFD3_STRIPBYTECOUNTS:
      *stripByteCounts = tagValue;
      break;
    case IFD3_CR2SLICES:
      saveFileOffset = ftell(file);
      fseek(file, tagValue, SEEK_SET);
      for(j=0; j<CR2_SLICES_LEN; j++)
        cr2Slices[j] = fiGet16(file);      
      fseek(file, saveFileOffset, SEEK_SET);
      break;
    default:
      ;
    }
  }
  return 0;
}

#define SENSORINFO_LEN (17+4)

/* 
 * check CR2 header and parses it to retrieve all TIFF tags value
 *  cr2data->tagList will contain a linked list with all TIFF tags
 *  mandatory data for jpeg decompression is also retrieved (stripOffsets, stripByteCounts)
 *  and for unslicing (cr2Slices)
 */
int cRaw2GetTags(struct cr2file *cr2) {

  unsigned long rawIFDOffset, firstIFD;
  unsigned long saveFileOffset, tagNb, tagValue;
  int n, i, j, len;
  unsigned short tagId, tagType;
  unsigned char cr2Header[CR2_HEADER_LENGTH];
  struct tiff_tag *tag_list, *tag;
  FILE* file;
	unsigned short sensorInfo[SENSORINFO_LEN];

  if (!cr2) {
    fprintf(stderr,"error cr2 is NULL\n");
    return -1;
  }
  file = cr2->file;	
  cr2->cr2Slices[0] = 0;  // for 20D    
   
  fseek(file, 0, SEEK_END);
  cr2->fileSize = ftell( file );
  fseek(file, 0, SEEK_SET);
  
  fread(cr2Header, sizeof(char), CR2_HEADER_LENGTH, file);
  if (strncmp((char*)cr2Header, "II*", 3)!=0 || strncmp((char*)(cr2Header+8), "CR", 2) || cr2Header[10]!=2 ) {
    fprintf(stderr, "cRaw2GetTags: invalid CR2 file\n");
    return -1; // error
  }
   
  rawIFDOffset = fiGet32(file);
   

  // find offset to first IFD (usually 0x10) 
  fseek(file, 4, SEEK_SET);
  firstIFD = fiGet32(file); 	
  fseek(file, firstIFD, SEEK_SET);
  cr2->tagList = tag_list = parseTiff(file, 0); // retrieve ALL TIFF tags (id, type, nb, value) 

  // one way to get interesting tags in IFD#3, we could also use cr2->tagList 

  //   analyseRawIFD( rawIFDOffset, file, &(cr2->stripOffsets), &(cr2->stripByteCounts), cr2->cr2Slices );

  fseek(file, rawIFDOffset, SEEK_SET);
  n = fiGet16(file); // number of tags in the RAW IFD
  for(i=0; i<n; i++) {
    tagId = fiGet16(file);
    tagType = fiGet16(file);
    tagNb = fiGet32(file);
    tagValue = fiGet32(file);
    /*if (cr2Verbose>1)
      printf("0x%04x %2d 0x%lx 0x%lx\n", tagId, tagType, tagNb, tagValue);*/
    switch(tagId) {
    case IFD3_STRIPOFFSETS:
      cr2->stripOffsets = tagValue;
      break;
    case IFD3_STRIPBYTECOUNTS:
      cr2->stripByteCounts = tagValue;
      break;
    case IFD3_CR2SLICES:
      saveFileOffset = ftell(file);
      fseek(file, tagValue, SEEK_SET);
      for(j=0; j<CR2_SLICES_LEN; j++)
        cr2->cr2Slices[j] = fiGet16(file);      
      fseek(file, saveFileOffset, SEEK_SET);
      break;
    default:
      ;
    }
  }
  n = fiGet16(file);
  if (n!=0) {
    if (cr2Verbose>0)
      printf("IFD#4 at 0x%x\n", n);
    analyseRawIFD( n, file, &(cr2->stripOffsetsB), &(cr2->stripByteCountsB), cr2->cr2SlicesB );

    if (cr2Verbose > 0) {
      printf("ImageB: stripOffsets=0x%lx, stripByteCounts=%ld, slices[]=", cr2->stripOffsetsB, cr2->stripByteCountsB);
      for (j = 0; j < CR2_SLICES_LEN; j++)
        printf("%d, ", cr2->cr2SlicesB[j]);
      printf("\n");
    }
  }
  
  tag = tag_list; // for WB values
  while(tag!=0) {
    /*if (cr2Verbose>1)
      printf("0x%04x %2d 0x%lx 0x%lx\n", tag->id, tag->type, tag->len, tag->val);*/
    if ( tag->context==0 ) {
      switch(tag->id) {
      case IFD0_CAMERA_MODEL: // camera model
        fseek(file, tag->val, SEEK_SET);
        cr2->model=malloc(tag->len+1);
        if (!(cr2->model)) {
          fprintf(stderr,"malloc cr2->model\n");
          return -1;
        }  
        fread(cr2->model, sizeof(char), tag->len, file); 
        cr2->model[tag->len]=0;
        break;
      }
    } // ifd#0
    else if ( tag->context==TIFFTAG_VAL_MAKERNOTE ) {
      switch(tag->id) {
      case MAKERNOTE_WHITE_BALANCE: // WB
        cRaw2GetWbValues(cr2, tag);
        break;
      case MAKERNOTE_MODEL_ID: // modelId
        cr2->modelId = tag->val;
        break;
      case MAKERNOTE_FIRMWARE_VERSION: // firmware
        fseek(file, tag->val, SEEK_SET);
        len = MIN( tag->len, CR2_FIRMWARE_LEN );
        fread(cr2->firmware, sizeof(char), len, file); 
        cr2->firmware[ len ] = 0;
        break;
      case MAKERNOTE_ASPECT_RATIO: // not_used
        fseek(file, tag->val, SEEK_SET);
        n = fiGet32(file);
        cr2->croppedImageWidth = fiGet32(file);
        cr2->croppedImageHeight = fiGet32(file);
        break;
      case MAKERNOTE_SENSOR_INFO: // sensor_info
        fseek(file, tag->val, SEEK_SET);
        for(i=0; i<SENSORINFO_LEN; i++)
          sensorInfo[i] = fiGet16(file); 			
        cr2->sensorWidth = sensorInfo[1];	
        cr2->sensorHeight = sensorInfo[2];	
        cr2->sensorLeftBorder = sensorInfo[5];	
        cr2->sensorTopBorder = sensorInfo[6];	
        cr2->sensorRightBorder = sensorInfo[7];	
        cr2->sensorBottomBorder = sensorInfo[8];	
        cr2->blackMaskLeftBorder = sensorInfo[9];	
        cr2->blackMaskTopBorder = sensorInfo[10];	
        cr2->blackMaskRightBorder = sensorInfo[11];	
        cr2->blackMaskBottomBorder = sensorInfo[12];	
       } // switch
    } // makernote
    else if( tag->context==TIFFTAG_VAL_EXIF ) {
      switch(tag->id) {
      case EXIF_IMAGE_WIDTH: // exifImageWidth
        cr2->exifImageWidth = (tag->val) & 0xffff;
        break;
      case EXIF_IMAGE_HEIGHT: // exifImageHeight
        cr2->exifImageHeight = (tag->val) & 0xffff;
        break;
      }
    } // exif
    tag = tag->next;
  }

	if ( ( (cr2->sensorRightBorder-cr2->sensorLeftBorder+1)!=cr2->exifImageWidth ) || ( ( cr2->sensorBottomBorder-cr2->sensorTopBorder+1)!=cr2->exifImageHeight) ) {
	  fprintf(stdout,"sensorinfo problem: %d-%d+1!=%d %d-%d+1!=%d\n", cr2->sensorRightBorder,cr2->sensorLeftBorder, cr2->exifImageWidth,
		  cr2->sensorBottomBorder, cr2->sensorTopBorder, cr2->exifImageHeight);
	  for(i=0; i<SENSORINFO_LEN; i++)
	    printf("si[%d]=%d ", i, sensorInfo[i] );
	  printf("\n");
  }
  if (cr2Verbose>0) {
    printf("SensorInfo: W=%d H=%d, L/T/R/B=%d/%d/%d/%d\n", cr2->sensorWidth, cr2->sensorHeight, 
     cr2->sensorLeftBorder, cr2->sensorTopBorder, cr2->sensorRightBorder, cr2->sensorBottomBorder);
    printf("ImageInfo: W=%d H=%d\n", cr2->exifImageWidth, cr2->exifImageHeight); 
  }
  return 0;
}

/*
 * cRaw2GetTagValue
 *
 * get TIFF tag content, located at 'tag->val' in 'file'. 
 * Allocate 'tag->len' * getTagTypeLen( 'tag->type' ) bytes in '*buf', must be freed by the caller
 * do not save the file pointer
 */
int cRaw2GetTagValue( struct tiff_tag *tag, unsigned char **buf, FILE* file ) {
  int typeLen; 
  unsigned long fileSize;
  
  if (tag==0 || buf==0 || file==0) {
    fprintf(stderr, "cRaw2GetTagValue: tag==0 || buf==0 || file==0\n");
    return -1;
  }
  typeLen = getTagTypeLen( tag->type );
  if (typeLen==0) {
    fprintf(stderr, "cRaw2GetTagValue: getTagTypeLen\n");
    return -1;
  }
  fseek( file, 0, SEEK_END );
  fileSize = ftell( file );
  if (tag->val + (tag->len * typeLen) > fileSize) {
    fprintf(stderr, "cRaw2GetTagValue: tag->val + (tag->len * typeLen)>fileSize\n");
    return -1;
  }
  *buf = malloc( tag->len * typeLen );
  if (!(*buf)) {
    fprintf(stderr, "cRaw2GetTagValue: malloc\n");
    return -1;
  }
  fseek(file, tag->val, SEEK_SET);
  if ( fread(*buf, sizeof(char), tag->len * typeLen, file) < (tag->len * typeLen) ) {
    fprintf(stderr, "cRaw2GetTagValue: fread short read\n");
    return -1;
  }
  return 0;
}


void cRaw2SetType( struct cr2file *cr2Data) {
  if (cr2Data->jpeg.hsf==1)
    cr2Data->type = RAW_TYPE_RGGB;
  else if (cr2Data->jpeg.vsf==1)
    cr2Data->type = RAW_TYPE_YUV422; // hsf==2, vsf==1
  else
    cr2Data->type = RAW_TYPE_YUV411; // hsf==2, vsf==2
}

static char *rawTypeStr[] = { "RGGB", "YUV422/sraw/sraw2", "YUV411/mraw/sraw1" };

char *cRaw2Info(struct cr2file *cr2Data) {
  static char buf[100];

  cRaw2SetType(cr2Data);

  if (cr2Data->jpeg.hsf>1)
    sprintf(buf, "Jpeg: %dx%d, %dbits, %d comp, %s, \"%s\", [%d, %d, %d]", cr2Data->jpeg.wide, cr2Data->jpeg.high, cr2Data->jpeg.bits, cr2Data->jpeg.ncomp, rawTypeStr[ cr2Data->type ], cr2Data->model,
      cr2Data->cr2Slices[0], cr2Data->cr2Slices[1], cr2Data->cr2Slices[2]);
  else
    sprintf(buf, "Jpeg: %dx%d, %dbits, %d comp, %s, \"%s\", [%d, %d, %d]", cr2Data->jpeg.wide*cr2Data->jpeg.ncomp, cr2Data->jpeg.high, cr2Data->jpeg.bits, cr2Data->jpeg.ncomp, rawTypeStr[ cr2Data->type ], 
      cr2Data->model, cr2Data->cr2Slices[0], cr2Data->cr2Slices[1], cr2Data->cr2Slices[2]);
	
	return buf;
}


// slices are vertical. from top left, then to the right, ends bottom right.
// they are filled the same way, slice per slice
/*
 * uses jpeg data, cr2Slices[] and fills unsliced, unslicedLen
 *
 * after unslicing, pixel are at definitive column and row in the image
 * but interpolation is still required, both for RGGB and YUV data
 * Cb and Cr data are converted to signed values
 */
/* Per camera oddies:
 * - 1Dm3 RAW: dcraw removes first 4 bytes of unsliced data, and add 4 zero bytes are the end (in python we would write: my_unsliced[4:] == dcraw_unsliced[:-4]) 
 */
 
/*
 * uses cr2->sensorHeight, cr2->sensorWidth
 * uses either cr2->jpeg or cr2->jpegB, cr2->cr2Slices or cr2->cr2SlicesB, unsliced or unslicedB 
 */ 
int cRaw2Unslice_new( struct cr2file *cr2, struct lljpeg *jpeg, unsigned short *cr2Slices, unsigned short **unsliced, unsigned long *unslicedLen ) {

  long line, column,  h, w, destBaseOffset;
  int slice, unslicedRowSize, comp, hv, endCol, startCol, sliceWidth;
  unsigned short compValue, _5dsrHFactor, _5dsrVFactor, lastSliceCol, sliceHeight;
  unsigned long j, rawLen, k;
/*
  for(j=0; j<3; j++)
    printf("%d ", cr2Slices[j]);
  printf("\n");  */
    
  if (!cr2) {
    fprintf(stderr,"cRaw2Unslice: cr2 pointer is null\n");
    return(1);
  }
  if (jpeg->rawBuffer==0) {
    fprintf(stderr,"cRaw2Unslice: jpeg rawbuffer is null\n");
    return(2);
  }
  h = jpeg->high / jpeg->vsf;    // vertical sampling factor
  w = jpeg->wide / jpeg->hsf;    // horizontal sampling factor
  if (cr2Slices[0]==0) { // 20D and 1D Mark 2 which have no 0xc640 tag
    cr2Slices[1] = 0;
    cr2Slices[2] = (w * jpeg->hvCount)& 0xffff; 
  }
  cRaw2SetType(cr2);
	
  *unslicedLen = jpeg->wide * jpeg->high * jpeg->ncomp;
  //printf("*unslicedLen %ld\n", *unslicedLen);
  
  if (jpeg->hsf==1 && *unslicedLen!=jpeg->rawBufferLen) // not true for YUV, because of hsf and vsf
    printf("cRaw2Unslice, RAW: cr2->unslicedLen (%ld) != cr2->jpeg.rawBufferLen (%ld)\n", *unslicedLen, jpeg->rawBufferLen );
/*    
  if (cr2->jpeg.hsf==1) {  // RAW
    if ( (cr2->jpeg.wide*cr2->jpeg.ncomp) != ( cr2->cr2Slices[0]*cr2->cr2Slices[1] + cr2->cr2Slices[2] ) )
      fprintf(stderr, "cRaw2Unslice, RAW: cr2->jpeg.wide*cr2->jpeg.ncomp (%d) != (cr2->cr2Slices[0]*cr2->cr2Slices[1] + cr2->cr2Slices[2]) (%d)\n", 
      cr2->jpeg.wide*cr2->jpeg.ncomp*cr2->jpeg.hsf, cr2->cr2Slices[0]*cr2->cr2Slices[1] + cr2->cr2Slices[2] );
  }
  else { 
    // only for sraw
    if ( (cr2->jpeg.wide*cr2->jpeg.hsf) != ( cr2->cr2Slices[0]*cr2->cr2Slices[1] + cr2->cr2Slices[2] ) )
      fprintf(stderr, "cRaw2Unslice, SRAW: cr2->jpeg.wide*cr2->jpeg.hsf (%d) != (cr2->cr2Slices[0]*cr2->cr2Slices[1] + cr2->cr2Slices[2]) (%d)\n", 
      cr2->jpeg.wide*cr2->jpeg.hsf, cr2->cr2Slices[0]*cr2->cr2Slices[1] + cr2->cr2Slices[2] );
 } 
*/    
  // not true for 5DS R  
  if (jpeg->hsf==1 && ( ((jpeg->wide * jpeg->ncomp)!=cr2->sensorWidth) || (jpeg->high!=cr2->sensorHeight ) ) ) 
    fprintf(stderr, "cRaw2Unslice, RAW: wide*ncomp (%d) != width (%d) or high (%d) != height (%d)\n", jpeg->wide * jpeg->ncomp, cr2->sensorWidth,
     jpeg->high, cr2->sensorHeight);
  
  *unsliced = (unsigned short*)calloc( *unslicedLen, sizeof(short) );
  if (!(*unsliced)) {
    fprintf(stderr,"cRaw2Unslice: calloc unsliced failed %p\n", *unsliced);
    return(3);
  }

  rawLen = h * w * jpeg->hvCount;
  //printf("cr2->jpeg.hvCount=%d\n", cr2->jpeg.hvCount); 
  if (rawLen!=jpeg->rawBufferLen)
    printf("rawLen=%ld cr2->jpeg.rawBufferLen=%ld cr2->unslicedLen=%ld\n", rawLen, jpeg->rawBufferLen, *unslicedLen);

  j=0;
  // iterates over destination buffer because slices are in this buffer

  sliceWidth = (cr2Slices[1]*jpeg->hsf) / jpeg->hvCount;  

  _5dsrHFactor = (jpeg->wide * jpeg->ncomp)/cr2->sensorWidth; // always 1, except for 5DS R (==2)
  _5dsrVFactor = cr2->sensorHeight/jpeg->high; // always 1, except for 5DS R (==2)
  //printf("_5dsr Factors: h=%d v=%d\n", _5dsrHFactor, _5dsrVFactor);
  if (jpeg->hsf==1 && (cr2->modelId==0x80000401 || cr2->modelId==0x80000349)) { // 5DS R and 5dm4
//  if (cr2->jpeg.hsf==1 && _5dsrHFactor==2 && _5dsrVFactor==2) {
    unslicedRowSize = cr2->sensorWidth; // or (cr2->jpeg.wide * cr2->jpeg.ncomp) / _5dsrHFactor
    lastSliceCol = cr2->sensorWidth/(_5dsrHFactor+_5dsrVFactor);
    sliceHeight = cr2->sensorHeight; // or cr2->jpeg.high * _5dsrVFactor
  }
  else { // normal
    lastSliceCol = jpeg->wide;
    unslicedRowSize = jpeg->wide * jpeg->ncomp;
    sliceHeight = jpeg->high;
  }
  //printf("unslicedRowSize=%d, lastSliceCol=%d sliceHeight=%d\n", unslicedRowSize, lastSliceCol, sliceHeight);
  for (slice=0; slice <= cr2Slices[0]; slice++) {
    
    startCol = slice * sliceWidth;
    if ( slice < cr2Slices[0] )
      endCol = (slice+1) * sliceWidth ; 
    else  
      endCol = lastSliceCol;    // last slice     

    if (cr2Verbose>0)
      printf("slice#%2d, scol=%5d, ecol=%5d, jpeg->rawBuffer[ j ]=%ld\n", slice, startCol, endCol, j);
      
    k = 0;  
    //printf("(endCol-startCol)*cr2->jpeg.high=%ld\n",(endCol-startCol)*cr2->jpeg.high);
//    for(line=0; line < cr2->jpeg.high; line+=cr2->jpeg.vsf)   
    for(line=0; line < sliceHeight; line+=jpeg->vsf)   //5DS R
      for(column=startCol; column<endCol; column+=jpeg->hsf) {

        for(comp=0; comp<jpeg->ncomp; comp++) {
          for(hv=0; hv<jpeg->hvComp[comp]; hv++) {
            if ( j >= rawLen ) { // for 70D, 6D, and 7Dm2 mraw
              fprintf(stderr, " j >= unslicedLen: line=%ld col=%ld slice=%d\n", line, column, slice);
              return -1;	 
            }
            compValue = jpeg->rawBuffer[ j++ ];
            destBaseOffset = line*unslicedRowSize + column*jpeg->ncomp;
            if ( jpeg->hsf == 1 )  // normal RAW
              (*unsliced)[ destBaseOffset + comp ] = compValue;
            else { // sRAW or mRAW
              if (comp==0) { // Y
                if (hv < jpeg->hsf)  // on same line, hsf>1, ==2 
                  (*unsliced)[ destBaseOffset + hv*jpeg->ncomp ] = compValue;           
                else  // on next line. hsv==1 or 2
                  (*unsliced)[ destBaseOffset + unslicedRowSize + (hv - jpeg->hsf)*jpeg->ncomp ] = compValue;       
              }  
              else // hsf>1 and comp>0, it is Cr or Cb
                (*unsliced)[ destBaseOffset + comp ] = (short)compValue - 16384; // convert it to signed
            } // sRAW or mRAW
            k++;
          } // for hv
        } // for comp
      } // for column
      // for line
      //printf("k=%ld\n",k);
  } // for slice
  return 0;

  
}
 
int cRaw2Unslice( struct cr2file *cr2 )  {
  unsigned long unslicedLen;
  
  cRaw2Unslice_new( cr2, &(cr2->jpeg), cr2->cr2Slices, &(cr2->unsliced), &(cr2->unslicedLen) );
  if (hasDualPixel(cr2)) {
      cRaw2Unslice_new( cr2, &(cr2->jpegB), cr2->cr2SlicesB, &(cr2->unslicedB), &unslicedLen );
      if (cr2->unslicedLen!=unslicedLen) 
        fprintf(stderr, "unslicedLenA = %ld != unslicedLenB = %ld\n", cr2->unslicedLen, unslicedLen);
  }
  
  return 0;
}

/*
 * reslice data from cr2->unsliced[] to sliced[]
 */
int cRaw2Slice( struct cr2file *cr2, unsigned short *sliced) {
  long line, column,  h, w, srcBaseOffset;
  int slice, unslicedRowSize, comp, hv, endCol, startCol, sliceWidth;
  unsigned short compValue, sliceHeight, lastSliceCol, _5dsrHFactor, _5dsrVFactor;
  unsigned long j, rawLen;

  if (!cr2) {
    fprintf(stderr,"cRaw2Slice: cr2 pointer is null\n");
    return(1);
  }

  if (cr2->unsliced==0) {
    fprintf(stderr,"cRaw2Slice: jpeg rawbuffer is null\n");
    return(2);
  }
  h = cr2->jpeg.high / cr2->jpeg.vsf;       // vertical sampling factor
  w = cr2->jpeg.wide / cr2->jpeg.hsf; // horizontal sampling factor
  rawLen = h * w * cr2->jpeg.hvCount;
  if (cr2->cr2Slices[0]==0) { // 20D
    cr2->cr2Slices[1] = 0;
    cr2->cr2Slices[2] = (w * cr2->jpeg.hvCount) & 0xffff; 
  }

  sliceWidth = (cr2->cr2Slices[1]*cr2->jpeg.hsf) / cr2->jpeg.hvCount;
  
  _5dsrHFactor = (cr2->jpeg.wide * cr2->jpeg.ncomp)/cr2->sensorWidth; // always 1, except for 5DS R (==2)
  _5dsrVFactor = cr2->sensorHeight/cr2->jpeg.high; // always 1, except for 5DS R (==2)
  if (cr2->jpeg.hsf==1 && cr2->modelId==0x80000401) { // 5DS R
    unslicedRowSize = cr2->sensorWidth; // or (cr2->jpeg.wide * cr2->jpeg.ncomp) / _5dsrHFactor
    lastSliceCol = cr2->sensorWidth/(_5dsrHFactor+_5dsrVFactor);
    sliceHeight = cr2->sensorHeight; // or cr2->jpeg.high * _5dsrVFactor
  }
  else { // normal
    lastSliceCol = cr2->jpeg.wide;
    unslicedRowSize = cr2->jpeg.wide * cr2->jpeg.ncomp;
    sliceHeight = cr2->jpeg.high;
  }
  
  j=0;
  // iterates over destination buffer because slices are in this buffer

  for (slice=0; slice <= cr2->cr2Slices[0]; slice++) {
    
    startCol = slice * sliceWidth;
    if ( slice < cr2->cr2Slices[0] )
      endCol = (slice+1) * sliceWidth ; 
    else  
      endCol = lastSliceCol ;    // last slice     

    if (cr2Verbose>0)
      printf("slice#%2d, scol=%5d, ecol=%5d, jpeg->rawBuffer[ j ]=%ld\n", slice, startCol, endCol, j);
      
    for(line=0; line < sliceHeight; line+=cr2->jpeg.vsf)   
      for(column=startCol; column<endCol; column+=cr2->jpeg.hsf) {
 
        for(comp=0; comp<cr2->jpeg.ncomp; comp++) {
          for(hv=0; hv<cr2->jpeg.hvComp[comp]; hv++) {
            if ( j >= rawLen ) {
              fprintf(stderr, " j >= unslicedLen: line=%ld col=%ld slice=%d\n", line, column, slice);
			        return -1;	 
			      }
            srcBaseOffset = line*unslicedRowSize + column*cr2->jpeg.ncomp;
            if ( cr2->jpeg.hsf == 1 )  // normal RAW
              compValue = cr2->unsliced[ srcBaseOffset + comp ];				           
						else { // sRAW or mRAW
              if (comp==0) { // Y
                if (hv < cr2->jpeg.hsf)  // on same line, hsf>1, ==2 
                  compValue = cr2->unsliced[ srcBaseOffset + hv*cr2->jpeg.ncomp ];           
                else  // on next line. hsv==1 or 2
                  compValue = cr2->unsliced[ srcBaseOffset + unslicedRowSize + (hv - cr2->jpeg.hsf)*cr2->jpeg.ncomp ];       
              }  
              else // hsf>1 and comp>0, it is Cr or Cb
                compValue = cr2->unsliced[ srcBaseOffset + comp ] + (1<<14); // convert it to signed
            }
            sliced[ j++ ] = compValue;

          } // for hv
        } // for comp
      } // for column
      // for line
  } // for slice
  return 0;
}

#define get16(b, i) (b[(i)]<<8 | b[(i)+1])

int cRaw2PatchAs(char* filename, struct cr2file *cr2, unsigned char *compressed, unsigned long compLen, unsigned char* dht) {
  unsigned long rawIFDOffset, val, jpegLen, filesize;
  unsigned long l, size, n, offset=0, i;
  int sosRead=0, dhtDelta;
  FILE* out;
	unsigned char *buf;
  unsigned short tagId;
	
	if (!dht) // no new DHT section
	  dhtDelta = 0;
	else 	
	  dhtDelta = get16(dht, 2) - cr2->jpeg.dhtSize; // how DHT size is changing ?
	
	out=(FILE*)fopen(filename, "wb+");
  if (!out) {
	  fprintf(stderr,"fopen error\n");
		return -1;
	}
	if (cr2Verbose>0)
	  printf("export as \"%s\"\n", filename);
	
	fseek(cr2->file, 0, SEEK_END);
	filesize = ftell(cr2->file);
	if (( cr2->stripOffsets+cr2->stripByteCounts) != filesize) // for 1Ds Mark II, 1D Mark IV and 1D X
	  fprintf(stderr, "warning, data after lossless jpeg [0x%lx-0x%lx] are truncated\n", cr2->stripOffsets+cr2->stripByteCounts, filesize);
		
	// find where is STRIP_BYTE_COUNTS to patch it
	fseek(cr2->file, CR2_HEADER_LENGTH, SEEK_SET);
  rawIFDOffset = fiGet32(cr2->file);
	fseek(cr2->file, rawIFDOffset, SEEK_SET);
	n = fiGet16(cr2->file); // number of tags in the RAW IFD
  for(i=0; i<n; i++) {
    tagId = fiGet16(cr2->file);
    fseek(cr2->file, 2+4+4, SEEK_CUR);
    switch(tagId) {
    case IFD3_STRIPOFFSETS:
      break;
    case IFD3_STRIPBYTECOUNTS: // to patch it
		  offset = ftell(cr2->file)-4;
      break;
    case IFD3_CR2SLICES:
      break;
    default:
      ;
    }
  }
//printf("offset=%ld\n",offset);
  buf=malloc(cr2->stripOffsets);
	if (!buf) {
	  fprintf(stderr,"malloc error\n");
		return -1;
	}
	// copy CR2 beginning, up to where to patch stripByteCounts
	fseek(cr2->file, 0, SEEK_SET);
  fread(buf, sizeof(char), offset, cr2->file); // before stripByteCounts
	fwrite(buf, sizeof(char), offset, out);
	jpegLen = dhtDelta+(cr2->jpeg.scanDataOffset - cr2->stripOffsets)+compLen+2; // jpeg header sections + scan data + JPEG_SOI
	//printf("jpegLen=%lx\n", jpegLen);
	fiPut32(jpegLen, out); // new stripByteCounts
	
	fseek(cr2->file, offset+4, SEEK_SET); // after stripByteCounts
  fread(buf, sizeof(char), cr2->stripOffsets-(offset+4), cr2->file);
	fwrite(buf, sizeof(char), cr2->stripOffsets-(offset+4), out);
	
	// keep jpeg headers in memory, always smaller than cr2->stripOffsets
  fread(buf, sizeof(char), cr2->jpeg.scanDataOffset - cr2->stripOffsets, cr2->file);
	if (!dht) 
    fwrite(buf, sizeof(char), cr2->jpeg.scanDataOffset - cr2->stripOffsets, out); // copy existing DHT section
	else { // patch jpeg headers
	  i=0;
    val = get16(buf,i);
    if (val!=JPEG_SOI) 
      return (-1); // ERROR
	  fwrite(buf, sizeof(char), 2, out); //JPEG_SOI
    i+=2;
	
    val = get16(buf,i); // keep same sections order, only patch the dht section
		i+=2;
    do {
      switch(val) {
      case JPEG_DHT:
        size = get16(buf,i); // original dht length
				i +=size; // i points to the next section in memory

        size = get16(dht, 2); // new dht length
				if (cr2Verbose>0)
				  printf("replacing jpeg DHT section\n");
        fwrite(dht, sizeof(char), size+2, out); // copy new DHT section 	
				break;
      case JPEG_SOF3:
      case JPEG_SOS:
        size = get16(buf,i);
        fwrite(buf+i-2, sizeof(char), size+2, out); // copy existing SOS section 	
        i +=size;
        if (val==JPEG_SOS)
				  sosRead = 1;
        break;
      default:
        if (cr2Verbose>0)
          printf("??? %lx\n", val);
      }
      val = get16(buf,i); 
		  i+=2;
    }while(!sosRead);  
  }
	free(buf);
	// jpeg compressed data
	if (compressed) {
    if (cr2Verbose>0)
       printf("patching jpeg compressed data\n");
	  fwrite(compressed, sizeof(char), compLen, out);
		fputc(JPEG_EOI>>8, out);
		fputc(JPEG_EOI&0xff, out);
  }
	else {
		l = cr2->stripByteCounts-(cr2->jpeg.scanDataOffset - cr2->stripOffsets);
    buf=malloc(l);
	  if (!buf) {
	    fprintf(stderr,"malloc error\n");
	    return -1;
	  }
	  fseek(cr2->file, cr2->jpeg.scanDataOffset, SEEK_SET); // should be needed...
    fread(buf, sizeof(char), l, cr2->file);
    fwrite(buf, sizeof(char), l, out);
		free(buf);
  }	
		
	fclose(out);
	return 0;
}

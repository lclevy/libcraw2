/* tiff.c
 * TIFF related functions
 * enough for Canon CR2
 * Intel "II" order only
 */
#include"tiff.h"
#include"craw2.h"
#include"rgb.h"
#include"yuv.h"

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

extern int cr2Verbose;

/*
#define TIFFTAG_TYPE_UCHAR  1
#define TIFFTAG_TYPE_STRING 2
#define TIFFTAG_TYPE_USHORT 3
#define TIFFTAG_TYPE_ULONG  4 
#define TIFFTAG_TYPE_URATIONAL  5 
#define TIFFTAG_TYPE_CHAR  6 
#define TIFFTAG_TYPE_BYTESEQ 7
#define TIFFTAG_TYPE_SHORT  8 
#define TIFFTAG_TYPE_LONG  9 
#define TIFFTAG_TYPE_RATIONAL  10 
#define TIFFTAG_TYPE_FLOAT4  11 
#define TIFFTAG_TYPE_FLOAT8  12 

*/




char * tiffTagType[]={ "uchar", "string", "ushort", "ulong", "urational", "char", "byteseq", "short", "long",
	"rational", "float4", "float8" };

// size in bytes for TIFFTAG_TYPE-1  
static int tagtypeLen[TIFFTAG_TYPE_MAX]={ 1, 1, 2, 4, 4+4, 1, 1, 2, 4, 4+4, 4, 8 };

int getTagTypeLen(int type) {
  if (type>0 && type<=TIFFTAG_TYPE_MAX)
    return(tagtypeLen[type-1]);
  else {
    fprintf(stderr,"getTagTypeLen: type is incorrect %d\n", type);
    return 0;
  }
}

// display tag value, may follow UCHAR, STRING and BYTESEQ
int displayTagValuesCsv( struct tiff_tag *tag, FILE* in, int follow, unsigned short max ) {
  int i, len;
  unsigned long savedOffset, fileSize;
  int c;

  savedOffset = ftell(in);
  fseek(in, 0, SEEK_END);
  fileSize = ftell( in );
  fseek(in, savedOffset, SEEK_SET);
  
  if ( tag->val > fileSize ) // can not follow value as pointer if outside of file content
    follow = 0;

  switch(tag->type) {
  case TIFFTAG_TYPE_UCHAR:
      savedOffset = ftell(in);
      fseek(in, tag->val, SEEK_SET);
      if (!follow)
        printf("0x%lx\n", tag->val);
      else {
        if (max != -1)
          len = MIN(tag->len, max);
        else
          len = tag->len;      
        for(i=0; i<len; i++)
          printf("%c", fgetc(in));
        putchar('\n');  
      }
      fseek(in, savedOffset, SEEK_SET);
    break;
  case TIFFTAG_TYPE_STRING:
      savedOffset = ftell(in);
      fseek(in, tag->val, SEEK_SET);
      if (!follow)
        printf("0x%lx\n", tag->val);
      else {
        if (max != -1)
          len = MIN(tag->len, max);
        else
          len = tag->len;     
        c = fgetc(in);  
        i=0;
        while(i<len && c) { // do not print trailing zero
          printf("%c", c );
          c = fgetc(in);  
          i++;
        }  
        putchar('\n');  
      }
      fseek(in, savedOffset, SEEK_SET);
    break;
  case TIFFTAG_TYPE_BYTESEQ:    
      savedOffset = ftell(in);
      fseek(in, tag->val, SEEK_SET);
      if (!follow)
        printf("0x%lx\n", tag->val);
      else {
        if (max != -1)
          len = MIN(tag->len, max);
        else
          len = tag->len;      
        for(i=0; i<len; i++)
          printf("%02x", fgetc(in));
        putchar('\n');  
      }
      fseek(in, savedOffset, SEEK_SET);
    break;
  case TIFFTAG_TYPE_USHORT: 
  case TIFFTAG_TYPE_ULONG: 
    printf("%lu\n", (long)fiGet32(in) );
    break;
  case TIFFTAG_TYPE_URATIONAL:
      savedOffset = ftell(in);
      fseek(in, tag->val, SEEK_SET);
      printf("%lu ", (unsigned long)fiGet32(in) );
      printf("/ %lu\n", (unsigned long)fiGet32(in) );
      fseek(in, savedOffset, SEEK_SET);      
    break;  
  case TIFFTAG_TYPE_RATIONAL:
      savedOffset = ftell(in);
      fseek(in, tag->val, SEEK_SET);
      printf("%ld ", (long)fiGet32(in) );
      printf("/ %ld\n", (long)fiGet32(in) );
      fseek(in, savedOffset, SEEK_SET);      
    break;  
  }
  return 0;
}

#define MAX_TIFF_CHAR_VALUES 30
#define MAX_TIFF_SHORT_VALUES 10
// used by -t option, to display TIFF tags entries
int displayTagValues( struct tiff_tag *tag, FILE* in, int allValues, int hexToo ) {
  int i, len, c;
  unsigned long savedOffset;
  
  switch(tag->type) {
  case TIFFTAG_TYPE_UCHAR:
    if (tag->len<5) 
      printf("%ld\n", tag->val);
    else {
      savedOffset = ftell(in);
      fseek(in, tag->val, SEEK_SET);
      if (!allValues)
        len = MIN(tag->len, MAX_TIFF_CHAR_VALUES);
      else
        len = tag->len;      
      for(i=0; i<len; i++)
        printf("%c", fgetc(in));
      if (!allValues && tag->len>MAX_TIFF_CHAR_VALUES)
        printf("...\n");
      else
        putchar('\n');        
      fseek(in, savedOffset, SEEK_SET);
    }  
    break;
  case TIFFTAG_TYPE_STRING:
    if (tag->len<5) 
      printf("%ld\n", tag->val);
    else {
      savedOffset = ftell(in);
      fseek(in, tag->val, SEEK_SET);
      if (!allValues)
        len = MIN(tag->len, MAX_TIFF_CHAR_VALUES);
      else
        len = tag->len;      
      c = fgetc(in);  
      i=0;
      while(i<len && c) { // do not print trailing zero
        printf("%c", c );
        c = fgetc(in);  
        i++;
      }   
      if (!allValues && tag->len>MAX_TIFF_CHAR_VALUES && c!=0)
        printf("...\n");
      else
        putchar('\n');        
      fseek(in, savedOffset, SEEK_SET);
    } 
    break;
  case TIFFTAG_TYPE_BYTESEQ:
    if (tag->len<5) 
      printf("%08lx\n", tag->val);
    else {
      savedOffset = ftell(in);
      fseek(in, tag->val, SEEK_SET);
      if (!allValues)
        len = MIN(tag->len, MAX_TIFF_CHAR_VALUES);
      else
        len = tag->len;      
      for(i=0; i<len; i++)
        printf("%02x", fgetc(in));
      if (!allValues && tag->len>MAX_TIFF_CHAR_VALUES)
        printf("...\n");
      else
        putchar('\n');        
      fseek(in, savedOffset, SEEK_SET);
    }  
    break;
  case TIFFTAG_TYPE_USHORT: 
  case TIFFTAG_TYPE_ULONG: 
    if (tag->len==1) {
      printf("%lu ", tag->val);
      if (hexToo) {
        if (tag->type==TIFFTAG_TYPE_USHORT)
          printf("(0x%04lx)\n", tag->val);
        else  
          printf("(0x%08lx)\n", tag->val);
      } 
      else
        putchar('\n');      
    }  
    else {
      if (!allValues)
        len = MIN(tag->len, MAX_TIFF_SHORT_VALUES);
      else 
        len = tag->len;    
      savedOffset = ftell(in);
      fseek(in, tag->val, SEEK_SET);
      if (tag->type==TIFFTAG_TYPE_USHORT) {
        for(i=0; i<len; i++)
          printf("%hu ", fiGet16(in) );
      }
      else    
        for(i=0; i<len; i++)
          printf("%lu ", (unsigned long)fiGet32(in) );
      if (!allValues && tag->len>MAX_TIFF_SHORT_VALUES)
        printf("...\n");
      else
        putchar('\n');        
      fseek(in, savedOffset, SEEK_SET);
    } // len>1
    break;
  case TIFFTAG_TYPE_URATIONAL:
      savedOffset = ftell(in);
      fseek(in, tag->val, SEEK_SET);
      printf("%lu ", (unsigned long)fiGet32(in) );
      printf("/ %lu\n", (unsigned long)fiGet32(in) );
      fseek(in, savedOffset, SEEK_SET);      
    break;  
  case TIFFTAG_TYPE_RATIONAL:
      savedOffset = ftell(in);
      fseek(in, tag->val, SEEK_SET);
      printf("%ld ", (long)fiGet32(in) );
      printf("/ %ld\n", (long)fiGet32(in) );
      fseek(in, savedOffset, SEEK_SET);      
    break;  
  default:  
    ; 
  }
  return 0;
}

void displayTag(struct tiff_tag *tag, FILE *in, int oneLine ) {
  if (oneLine)
    printf( "%5d 0x%06lx %5d/0x%-4x %9s(%d)*%-6ld %9lu/0x%-lx, ", 
      tag->context, tag->offset, tag->id, tag->id, tiffTagType[tag->type-1], tag->type, tag->len, tag->val, tag->val);
    displayTagValues(tag, in, 0, 1);
}

void displayTagCsv(struct tiff_tag *tag, FILE *in, int follow, int max ) {
  printf( "%d, 0x%lx, %d, %d, %ld, 0x%lx, ", 
      tag->context, tag->offset, tag->id, tag->type, tag->len, tag->val);
  displayTagValuesCsv(tag, in, follow, max);
}


struct tiff_tag * newTiffTag(struct tiff_tag *last)
{
  struct tiff_tag* newtag;

  newtag = (struct tiff_tag*)malloc(sizeof(struct tiff_tag));
  if (!newtag) {
    fprintf(stderr, "addTiffTag: malloc error\n");
    return 0;
  }
  newtag->next = 0;
  
  if (last!=0) // the linked list is empty
    last->next = newtag;

  return newtag;
}

// II mode only
void putEntry(FILE *out, unsigned short id, unsigned short type, unsigned long len, unsigned long val) {
  fiPut16(id, out);
  fiPut16(type, out);
  fiPut32(len, out);
  fiPut32(val, out);
}

void getEntry(FILE *in, struct tiff_tag *entry)
{
    entry->offset = ftell(in);
    entry->id = fiGet16(in);
    entry->type = fiGet16(in);
    entry->len = fiGet32(in);
    entry->val = fiGet32(in);
 }
/*
int fWriteFloat(float d, FILE* out) {
  float d2=d;
	//printf("%f\n", d);
  fwrite( &d2, sizeof(float), 1, out);
  return 0;
}
*/
void fiPutRational(FILE* out, unsigned long n, unsigned long d) {
	fiPut32( n, out);
	fiPut32( d, out);
}

/*
 * export RGGB data as TIFF, like "dcraw -T -4 -D" 
 *
 * can be verified using "exiftool.exe -e -H -u rggb.tiff"
 */
int exportRggbTiff(struct cr2file *cr2, char *filename) {
  FILE* out;
	unsigned long ifdOffset, imageLen, nextOffset;
  int nEntries;
  unsigned short line, rowWidth;
  unsigned short* pixels; 
	char buf[50];
  
	out=fopen(filename, "wb");
	if (!out) {
	  fprintf(stderr, "exportRggbTiff: fopen\n");
		return -1;
	}
	sprintf(buf, "libcraw2 v%s", CRAW2_VERSION_STR);

  if (cr2Verbose)
    printf("exporting %s\n", filename);
  
	fputs("II", out);
	fiPut16(42, out);
 
  imageLen = cr2->width * cr2->height;

  rowWidth = cr2->sensorWidth;
	ifdOffset = imageLen*sizeof(short)+8;

  fiPut32( ifdOffset, out);	// offset to IFD
  pixels = cr2->unsliced + (cr2->tBorder+cr2->vShift)*rowWidth + cr2->lBorder; // start of image in raw data, cropping using borders & shift values
  for(line = cr2->tBorder+cr2->vShift; line <= cr2->bBorder+cr2->vShift; line++) {
    fwrite( pixels, sizeof(short), cr2->width, out); // image data, line per line
    pixels = pixels + rowWidth;
  }
  nEntries = 16;
  nextOffset = ifdOffset+2+nEntries*12+4;
  fiPut16(nEntries, out);	// number of tag entries
  putEntry( out, TIFFTAG_VAL_IMAGEWIDTH, TIFFTAG_TYPE_USHORT, 1, cr2->width);
  putEntry( out, TIFFTAG_VAL_IMAGEHEIGHT, TIFFTAG_TYPE_USHORT, 1, cr2->height);
  putEntry( out, TIFFTAG_VAL_BITSPERSAMPLES, TIFFTAG_TYPE_USHORT, 1, 16);
  putEntry( out, MAKERNOTE_MODEL_ID, TIFFTAG_TYPE_ULONG, 1, cr2->modelId ); // non standard, could be better in a MakerNote
  
  putEntry( out, TIFFTAG_VAL_RAW_WB_RATIO, TIFFTAG_TYPE_USHORT, 4, nextOffset); // non standard, stored in Makernote, 0x4001
  nextOffset = nextOffset + 4*getTagTypeLen(TIFFTAG_TYPE_USHORT);
  
  putEntry( out, TIFFTAG_VAL_COMPRESSION, TIFFTAG_TYPE_USHORT, 1, 1); // 1=uncompressed
  putEntry( out, TIFFTAG_VAL_PHOTOMETRICINTERPRETATION, TIFFTAG_TYPE_USHORT, 1, 1); // 2=RGB, 1=BlackIsZero
  putEntry( out, TIFFTAG_VAL_FILLORDER, TIFFTAG_TYPE_USHORT, 1, 1); // 1=normal
  putEntry( out, TIFFTAG_VAL_PLANARCONFIGURATION, TIFFTAG_TYPE_USHORT, 1, 1); // 1=chunky
  putEntry( out, TIFFTAG_VAL_STRIPOFFSET, TIFFTAG_TYPE_ULONG, 1, 8);
  putEntry( out, TIFFTAG_VAL_STRIPBYTECOUNTS, TIFFTAG_TYPE_ULONG, 1, ifdOffset-8);
  putEntry( out, TIFFTAG_VAL_SAMPLESPERPIXEL, TIFFTAG_TYPE_USHORT, 1, 1); 
  putEntry( out, TIFFTAG_VAL_XRESOLUTION, TIFFTAG_TYPE_URATIONAL, 1, nextOffset ); // 0x0000012c 0x00000001 
  nextOffset = nextOffset + getTagTypeLen(TIFFTAG_TYPE_URATIONAL);
  
  putEntry( out, TIFFTAG_VAL_YRESOLUTION, TIFFTAG_TYPE_URATIONAL, 1, nextOffset); // 0x0000012c 0x00000001
  nextOffset = nextOffset + getTagTypeLen(TIFFTAG_TYPE_URATIONAL);;

  putEntry( out, TIFFTAG_VAL_RESOLUTIONUNIT, TIFFTAG_TYPE_USHORT, 1, 2); // 2=inches, 3=cm  	
  putEntry( out, TIFFTAG_VAL_SOFTWARE, TIFFTAG_TYPE_STRING, (unsigned long)strlen(buf), nextOffset);   	
  fiPut32( 0, out); // end of IFD
  
  //TIFFTAG_VAL_RAW_WB_RATIO
  fiPut16(cr2->wbRggb[0], out);
  fiPut16(cr2->wbRggb[1], out);
  fiPut16(cr2->wbRggb[2], out);
  fiPut16(cr2->wbRggb[3], out);

  // TIFFTAG_VAL_XRESOLUTION
  fiPutRational( out, 300, 1 );
  // TIFFTAG_VAL_YRESOLUTION
  fiPut32( 0x12c, out);
  fiPut32( 1, out);
  fprintf(out, "%s", buf); // libcraw version
  fclose(out); 

  return 0;
}

/*
 * export RGGB data as TIFF, like "dcraw -T -4 -D" 
 *
 * can be verified using "exiftool.exe -e -H -u rggb.tiff"
 */
int export16BitsBufferAsTiff( char *filename, unsigned short *buffer, unsigned long h, unsigned long w) {
  FILE* out;
  unsigned long ifdOffset, imageLen, nextOffset;
  int nEntries;
//  unsigned short line, rowWidth;
  unsigned short* pixels; 
  char buf[50];
  
  out=fopen(filename, "wb");
  if (!out) {
    fprintf(stderr, "export16BitsBufferAsTiff: fopen\n");
    return -1;
  }
  sprintf(buf, "libcraw2 v%s", CRAW2_VERSION_STR);

  if (cr2Verbose)
    printf("exporting %s\n", filename);
  
  fputs("II", out);
  fiPut16(42, out);

  imageLen = h * w;
  ifdOffset = imageLen*sizeof(short)+8;

  fiPut32( ifdOffset, out);	// offset to IFD
  pixels = buffer;
  fwrite( pixels, sizeof(short), imageLen, out); // image data, line per line

  nEntries = 14;
  nextOffset = ifdOffset+2+nEntries*12+4;
  fiPut16(nEntries, out);	// number of tag entries
  putEntry( out, TIFFTAG_VAL_IMAGEWIDTH, TIFFTAG_TYPE_USHORT, 1, w);
  putEntry( out, TIFFTAG_VAL_IMAGEHEIGHT, TIFFTAG_TYPE_USHORT, 1, h);
  putEntry( out, TIFFTAG_VAL_BITSPERSAMPLES, TIFFTAG_TYPE_USHORT, 1, 16);
  //putEntry( out, MAKERNOTE_MODEL_ID, TIFFTAG_TYPE_ULONG, 1, cr2->modelId ); // non standard, could be better in a MakerNote
  
  putEntry( out, TIFFTAG_VAL_COMPRESSION, TIFFTAG_TYPE_USHORT, 1, 1); // 1=uncompressed
  putEntry( out, TIFFTAG_VAL_PHOTOMETRICINTERPRETATION, TIFFTAG_TYPE_USHORT, 1, 1); // 2=RGB, 1=BlackIsZero
  putEntry( out, TIFFTAG_VAL_FILLORDER, TIFFTAG_TYPE_USHORT, 1, 1); // 1=normal
  putEntry( out, TIFFTAG_VAL_PLANARCONFIGURATION, TIFFTAG_TYPE_USHORT, 1, 1); // 1=chunky
  putEntry( out, TIFFTAG_VAL_STRIPOFFSET, TIFFTAG_TYPE_ULONG, 1, 8);
  putEntry( out, TIFFTAG_VAL_STRIPBYTECOUNTS, TIFFTAG_TYPE_ULONG, 1, ifdOffset-8);
  putEntry( out, TIFFTAG_VAL_SAMPLESPERPIXEL, TIFFTAG_TYPE_USHORT, 1, 1); 
  putEntry( out, TIFFTAG_VAL_XRESOLUTION, TIFFTAG_TYPE_URATIONAL, 1, nextOffset ); // 0x0000012c 0x00000001 
  nextOffset = nextOffset + getTagTypeLen(TIFFTAG_TYPE_URATIONAL);
  
  putEntry( out, TIFFTAG_VAL_YRESOLUTION, TIFFTAG_TYPE_URATIONAL, 1, nextOffset); // 0x0000012c 0x00000001
  nextOffset = nextOffset + getTagTypeLen(TIFFTAG_TYPE_URATIONAL);

  putEntry( out, TIFFTAG_VAL_RESOLUTIONUNIT, TIFFTAG_TYPE_USHORT, 1, 2); // 2=inches, 3=cm  	
  putEntry( out, TIFFTAG_VAL_SOFTWARE, TIFFTAG_TYPE_STRING, (unsigned long)strlen(buf), nextOffset);
  fiPut32( 0, out); // end of IFD
  
  // TIFFTAG_VAL_XRESOLUTION
  fiPutRational( out, 300, 1 );
  // TIFFTAG_VAL_YRESOLUTION
  fiPut32( 0x12c, out);
  fiPut32( 1, out);
  fprintf(out, "%s", buf); // libcraw version
  fclose(out); 

  return 0;
}

/*
 * export RGB 16 bits data as TIFF 
 * 
 * from RGGB: ncomp == 4 and blue at offset 3
 * from YUV: ncomp == 3, and blue at offset 2 
 */
int exportRgbTiff(struct cr2image *image, char *filename, int g2_b_inversed) {
  FILE* out;
  unsigned long ifdOffset, imageLen;
  int nEntries;
  unsigned short line, column;
  char buf[50]; // for libcraw2 version text
  int i, ncomp;
  
  out=fopen(filename, "wb");
  if (!out) {
    fprintf(stderr, "exportRgbTiff: fopen\n");
    return -1;
  }
  sprintf(buf, "libcraw2 v%s", CRAW2_VERSION_STR);

  if (cr2Verbose)
    printf("exportRgbTiff %s\n", filename);
  
  fputs("II", out);
  fiPut16(42, out); // TIFF magic number, and "Answer to The Ultimate Question of Life, the Universe, and Everything"

  if (image->isRGGB)
    ncomp = 4;
  else
    ncomp = 3; // YCbCr
    
  imageLen = image->width * image->height*3;
  ifdOffset = imageLen*sizeof(short)+8;
  fiPut32( ifdOffset, out);	// offset to IFD
  for(line=0; line < image->height; line++)
    for(column=0; column < image->width; column++) {
      fwrite( image->pixels + line*image->width*ncomp + column*ncomp   , sizeof(short), 1, out); // R or Y
      fwrite( image->pixels + line*image->width*ncomp + column*ncomp +1, sizeof(short), 1, out); // interpolated G1 or Cb 
      if (!g2_b_inversed || ncomp == 3)
        fwrite( image->pixels + line*image->width*ncomp + column*ncomp +ncomp-1, sizeof(short), 1, out); // B or Cr
      else  
        fwrite( image->pixels + line*image->width*ncomp + column*ncomp +2, sizeof(short), 1, out); // B inversed with G2, to ease NumPy computations
    } // for column

  nEntries = 14;
  fiPut16(nEntries, out);	// number of tag entries
  putEntry( out, TIFFTAG_VAL_IMAGEWIDTH, TIFFTAG_TYPE_USHORT, 1, image->width);
  putEntry( out, TIFFTAG_VAL_IMAGEHEIGHT, TIFFTAG_TYPE_USHORT, 1, image->height);
  putEntry( out, TIFFTAG_VAL_BITSPERSAMPLES, TIFFTAG_TYPE_USHORT, 3, ifdOffset+2+nEntries*12+4 +8+8);
  putEntry( out, TIFFTAG_VAL_COMPRESSION, TIFFTAG_TYPE_USHORT, 1, 1); // 1=uncompressed
  putEntry( out, TIFFTAG_VAL_PHOTOMETRICINTERPRETATION, TIFFTAG_TYPE_USHORT, 1, 2); // 2=RGB, 1=BlackIsZero
  putEntry( out, TIFFTAG_VAL_FILLORDER, TIFFTAG_TYPE_USHORT, 1, 1); // 1=normal
  putEntry( out, TIFFTAG_VAL_PLANARCONFIGURATION, TIFFTAG_TYPE_USHORT, 1, 1); // 1=chunky
  putEntry( out, TIFFTAG_VAL_STRIPOFFSET, TIFFTAG_TYPE_ULONG, 1, 8);
  putEntry( out, TIFFTAG_VAL_STRIPBYTECOUNTS, TIFFTAG_TYPE_ULONG, 1, ifdOffset-8);
  putEntry( out, TIFFTAG_VAL_SAMPLESPERPIXEL, TIFFTAG_TYPE_USHORT, 1, 3); 
  putEntry( out, TIFFTAG_VAL_XRESOLUTION, TIFFTAG_TYPE_URATIONAL, 1, ifdOffset+2+nEntries*12+4 ); // 0x0000012c 0x00000001 
  putEntry( out, TIFFTAG_VAL_YRESOLUTION, TIFFTAG_TYPE_URATIONAL, 1, ifdOffset+2+nEntries*12+4 +8); // 0x0000012c 0x00000001
  putEntry( out, TIFFTAG_VAL_RESOLUTIONUNIT, TIFFTAG_TYPE_USHORT, 1, 2); // 2=inches, 3=cm  	
  putEntry( out, TIFFTAG_VAL_SOFTWARE, TIFFTAG_TYPE_STRING, (unsigned long)strlen(buf), ifdOffset+2+nEntries*12+4 +8+8 +3*2);
  fiPut32( 0, out); // end of IFD
  // TIFFTAG_VAL_XRESOLUTION
  fiPutRational( out, 300, 1 );
  // TIFFTAG_VAL_YRESOLUTION
  fiPutRational( out, 300, 1 );
  // TIFFTAG_VAL_BITSPERSAMPLES
  for(i=0; i<3; i++);
  fiPut16(16, out);
  fiPut16(16, out);
  fiPut16(16, out);
  // TIFFTAG_VAL_SOFTWARE 
  fprintf(out, "%s", buf); // libcraw version
  fclose(out); 

  return 0;
}

/*
 * export YCbCr data using TIFF class Y, but with 16bits (non standard)
 * the standard is 8bits only, with Strip Offsets and Strip Bytes Count as lists of pointer/length in text: non sense (maybe legacy)
 * if 'packed' is true, 'interpolate' value is ignored (because interpolated values are removed by packing), else 'interpolated' is taken into account
 * store enough data (rggb and sraw wb ratios, modelid, firmware version, jpeg hsf and vsf) for yuv2rgb computation after reload
 */
int exportYuvTiff(struct cr2file *cr2, char *filename, int interpolate, int packed) {
  FILE* out;
  unsigned short *packedData=0;
  unsigned long packedLen, pl, i;
  unsigned long ifdOffset, nextOffset;
  int nEntries;
	char buf[50]; // for libcraw2 version text
  //long white = 1<<(cr2->jpeg.bits);
  unsigned short primaryChroma[6] = {640, 330, 300, 600, 150, 60 };

  if (cr2Verbose)
    printf("exportYuvTiff %s\n", filename);
  
  out=fopen(filename, "wb");
  if (!out) {
    fprintf(stderr, "exportYuvTiff: fopen\n");
    return -1;
  }
  fputs("II", out);
  fiPut16(42, out);
  if (!cr2->unsliced) {
    fprintf(stderr, "exportYuvTiff: data must be unsliced first\n");
    return -1;
  }
	sprintf(buf, "libcraw2 v%s", CRAW2_VERSION_STR);

  if (packed) { // stored in packed form, with no empty pixels
    pl = cr2->jpeg.wide*cr2->jpeg.high + ( (2*cr2->jpeg.wide*cr2->jpeg.high) / (cr2->jpeg.hsf*cr2->jpeg.vsf) );
    packedData = malloc( cr2->jpeg.rawBufferLen*sizeof(short) );
    if (!packedData) {
      fprintf(stderr,"exportYuvTiff: malloc packed\n");
      return -1;
    }
    yuvPack(cr2, packedData, &packedLen);
    if (pl!=packedLen)
      printf("packed YUV data length=%ld (%ld expected)\n", packedLen, pl);
    ifdOffset = packedLen*sizeof(short)+8;
    fiPut32( ifdOffset, out);	
    fwrite(packedData, sizeof(short), packedLen, out);	// work only on little endian machines, but faster.
    
    free(packedData);
  } // packed
  
  else { // stored not packed, and interpolate missing values if requested (interpolate!=0)
    if (interpolate)
      yuvInterpolate( cr2 );
    ifdOffset = cr2->unslicedLen*sizeof(short)+8;
    fiPut32( ifdOffset, out);	
    fwrite(cr2->unsliced, sizeof(short), cr2->unslicedLen, out);	// work only on little endian machines, but faster.
  } // interpolate
	
    nEntries = 24;
    nextOffset = ifdOffset+2+nEntries*12+4;
    fiPut16(nEntries, out);	// number of tag entries
    putEntry( out, TIFFTAG_VAL_IMAGEWIDTH, TIFFTAG_TYPE_USHORT, 1, cr2->jpeg.wide);
    putEntry( out, TIFFTAG_VAL_IMAGEHEIGHT, TIFFTAG_TYPE_USHORT, 1, cr2->jpeg.high);
    putEntry( out, MAKERNOTE_MODEL_ID, TIFFTAG_TYPE_ULONG, 1, cr2->modelId ); // non standard, could be better in a MakerNote
    putEntry( out, TIFFTAG_VAL_SRAW_WB_RATIO, TIFFTAG_TYPE_USHORT, 4, nextOffset);  // non standard, stored in Makernote, 0x4001
    nextOffset = nextOffset + 4*getTagTypeLen(TIFFTAG_TYPE_USHORT);
    
    putEntry( out, TIFFTAG_VAL_RAW_WB_RATIO, TIFFTAG_TYPE_USHORT, 4, nextOffset); // non standard, stored in Makernote, 0x4001
    nextOffset = nextOffset + 4*getTagTypeLen(TIFFTAG_TYPE_USHORT);
    
    putEntry( out, TIFFTAG_VAL_ORIGINAL_HSFVSF, TIFFTAG_TYPE_ULONG, 1, (cr2->jpeg.hsf<<16) | cr2->jpeg.vsf); // non standard, stored in jpeg
    
    putEntry( out, TIFFTAG_VAL_BITSPERSAMPLES, TIFFTAG_TYPE_USHORT, 3, nextOffset); // mandatory
    nextOffset = nextOffset + 3*getTagTypeLen(TIFFTAG_TYPE_USHORT);
    
    putEntry( out, TIFFTAG_VAL_COMPRESSION, TIFFTAG_TYPE_USHORT, 1, 1); // 1=uncompressed, mandatory
    putEntry( out, TIFFTAG_VAL_PHOTOMETRICINTERPRETATION, TIFFTAG_TYPE_USHORT, 1, 6); // 6=YCbCr, mandatory
    putEntry( out, TIFFTAG_VAL_FILLORDER, TIFFTAG_TYPE_USHORT, 1, 1); // 1=normal
    putEntry( out, TIFFTAG_VAL_PLANARCONFIGURATION, TIFFTAG_TYPE_USHORT, 1, 1); // 1=chunky
    
    if (packed) { 
      if (cr2->jpeg.vsf>1)
        putEntry( out, TIFFTAG_VAL_YCBCRSUBSAMPLING, TIFFTAG_TYPE_USHORT, 2, 0x00010004); // (11=444), (21=422), 41=411
      else
        putEntry( out, TIFFTAG_VAL_YCBCRSUBSAMPLING, TIFFTAG_TYPE_USHORT, 2, 0x00010002); // 21=422
    }
    else
      putEntry( out, TIFFTAG_VAL_YCBCRSUBSAMPLING, TIFFTAG_TYPE_USHORT, 2, 0x00010001); // 11=444, (21=422, 41=411)
      
	putEntry( out, TIFFTAG_VAL_WHITEPOINT, TIFFTAG_TYPE_URATIONAL, 2, nextOffset); 
  nextOffset = nextOffset + 2*getTagTypeLen(TIFFTAG_TYPE_URATIONAL);
  
  putEntry( out, TIFFTAG_VAL_PRIMARYCHROMACITIES, TIFFTAG_TYPE_URATIONAL, 6, nextOffset);
  nextOffset = nextOffset + 6*getTagTypeLen(TIFFTAG_TYPE_URATIONAL);
  
	putEntry( out, TIFFTAG_VAL_STRIPOFFSET, TIFFTAG_TYPE_ULONG, 1, 8); // unlike strange 8bits fashion, but like RGB (non standard, also because 16bits)
	putEntry( out, TIFFTAG_VAL_STRIPBYTECOUNTS, TIFFTAG_TYPE_ULONG, 1, ifdOffset-8); // unlike strange 8bits fashion, but like RGB (non standard, also because 16bits)

	putEntry( out, TIFFTAG_VAL_SAMPLESPERPIXEL, TIFFTAG_TYPE_USHORT, 1, 3); //mandatory
	putEntry( out, TIFFTAG_VAL_ROWSPERSTRIP, TIFFTAG_TYPE_USHORT, 1, 1); 
	putEntry( out, TIFFTAG_VAL_XRESOLUTION, TIFFTAG_TYPE_URATIONAL, 1, nextOffset); // 300/1 
  nextOffset = nextOffset + 1*getTagTypeLen(TIFFTAG_TYPE_URATIONAL);
  
	putEntry( out, TIFFTAG_VAL_YRESOLUTION, TIFFTAG_TYPE_URATIONAL, 1, nextOffset); // 300/1
  nextOffset = nextOffset + 1*getTagTypeLen(TIFFTAG_TYPE_URATIONAL);
  
	putEntry( out, TIFFTAG_VAL_RESOLUTIONUNIT, TIFFTAG_TYPE_USHORT, 1, 2); // 2=inches, 3=cm  	
	putEntry( out, TIFFTAG_VAL_YCBCRPOSITIONING,  TIFFTAG_TYPE_USHORT, 1, 1); // 1=centered 
  
  /*putEntry( out, TIFFTAG_VAL_REFERENCEBLACKWHITE, TIFFTAG_TYPE_URATIONAL, 6, nextOffset);
    nextOffset = nextOffset + 6*getTagTypeLen(TIFFTAG_TYPE_URATIONAL);
  */
  putEntry( out, MAKERNOTE_FIRMWARE_VERSION, TIFFTAG_TYPE_STRING, (unsigned long)strlen(cr2->firmware), nextOffset);
  nextOffset = nextOffset + (unsigned long)strlen(cr2->firmware);

  putEntry( out, TIFFTAG_VAL_SOFTWARE, TIFFTAG_TYPE_STRING, (unsigned long)strlen(buf), nextOffset);

	fiPut32( 0, out); // end of IFD
  
  // TIFFTAG_VAL_SRAW_WB_RATIO
  //for(i=0; i<4; i++)
    fiPut16(cr2->srawRggb[0], out); // BUG with for(): FIXIT 
    fiPut16(cr2->srawRggb[1], out);
    fiPut16(cr2->srawRggb[2], out);
    fiPut16(cr2->srawRggb[3], out);
  // TIFFTAG_VAL_RAW_WB_RATIO  
  //for(i=0; i<4; i++)
    fiPut16(cr2->wbRggb[0], out);
    fiPut16(cr2->wbRggb[1], out);
    fiPut16(cr2->wbRggb[2], out);
    fiPut16(cr2->wbRggb[3], out);
  
  // TIFFTAG_VAL_SAMPLESPERPIXEL
  fiPut16(15, out);
  fiPut16(15, out);
	fiPut16(15, out);

  /*
  // TIFFTAG_VAL_REFERENCEBLACKWHITE 
	fiPut32( 0, out); //0
	fiPut32( 1, out); //0
	fiPut32( white, out); //1
	fiPut32( 1, out); //1
	fiPut32( white/2, out); // 2
	fiPut32( 1, out); //2
	fiPut32( white, out); // 3
	fiPut32( 1, out); //3
	fiPut32( white/2, out); // 4
	fiPut32( 1, out); //4
	fiPut32( white, out); // 5
	fiPut32( 1, out); //5
  */
  // TIFFTAG_VAL_WHITEPOINT, (mandatory, rational, 3127/10000, 3290/10000 see TIFF6.0 section 20) 
  fiPutRational( out, 3127, 10000 );
  fiPutRational( out, 3290, 10000 );
	// TIFFTAG_VAL_PRIMARYCHROMACITIES (optional, rational, r=640/1000, 330/1000, b=300/1000, 600/1000, g=150/1000, 60/1000 see TIFF6.0 section 20)
  for(i=0; i<6; i++)
    fiPutRational( out, primaryChroma[i], 1000 );
    
  // TIFFTAG_VAL_XRESOLUTION
  fiPutRational( out, 300, 1 );
	// TIFFTAG_VAL_YRESOLUTION
  fiPutRational( out, 300, 1 );
  
  // MAKERNOTE_FIRMWARE_VERSION
  fprintf(out, "%s", cr2->firmware);
  // TIFFTAG_VAL_SOFTWARE 
  fprintf(out, "%s", buf); // libcraw version
	fclose(out); 

	return 0;
}

/*
 * import a YUV 16bits saved by exportYuvTiff
 * custom TIFF format!
 * could be 411, 444, or 422
 * reload enough info for yuv2rgb computation
 */
int yuvTiffOpen(struct cr2file *cr2, char *filename)
{
  FILE *in;
  struct tiff_tag *tags, *tag;
  unsigned long stripOffset=0, stripBytesCounts=0, subSampling=0, unPackedLen;
  int len, i;
  unsigned short *yuvData;
  unsigned long value;
  
  in=(FILE*)fopen(filename, "rb");
  if (!in) {
    fprintf(stderr,"yuvTiffOpen: fopen %s\n", filename);
    return -1;
  }
  if (cr2Verbose)
    printf("yuvTiffOpen %s\n", filename);

  tags = parseTiff(in, 0);
  tag = tags;
  while(tag->next >0) {
  
    switch(tag->id) {
    case TIFFTAG_VAL_SRAW_WB_RATIO:
      if ( tag->len != 4 ) {
        fprintf(stderr,"TIFFTAG_VAL_SRAW_WB_RATIO len!=4\n");
        return -1;
      }
      fseek(in, tag->val, SEEK_SET);
      for(i=0; i<4; i++)
        cr2->srawRggb[i] = fiGet16(in);
      break;
    case TIFFTAG_VAL_RAW_WB_RATIO:
      if ( tag->len != 4 ) {
        fprintf(stderr,"TIFFTAG_VAL_RAW_WB_RATIO len!=4\n");
        return -1;
      }
      fseek(in, tag->val, SEEK_SET);
      for(i=0; i<4; i++)
        cr2->wbRggb[i] = fiGet16(in);
      break;
    case TIFFTAG_VAL_IMAGEWIDTH:
      cr2->jpeg.wide = (unsigned short)tag->val;
      break;
    case TIFFTAG_VAL_IMAGEHEIGHT:
      cr2->jpeg.high = (unsigned short)tag->val;
      break;
    case TIFFTAG_VAL_BITSPERSAMPLES:
      if ( tag->len != 3 ) {
        fprintf(stderr,"TIFFTAG_VAL_BITSPERSAMPLES len!=3\n");
        return -1;
      }
      fseek(in, tag->val, SEEK_SET);
      value = fiGet16(in);
      if (value!=15) {
        fprintf(stderr,"TIFFTAG_VAL_BITSPERSAMPLES val!=15\n");
        return -1;
      }
      cr2->jpeg.bits = 15;
      break;
    case TIFFTAG_VAL_COMPRESSION:
      if (tag->val!=1) {
        fprintf(stderr,"TIFFTAG_VAL_COMPRESSION, only uncompressed (1) is supported\n");
        return -1;
      }
      break;
    case TIFFTAG_VAL_PHOTOMETRICINTERPRETATION:
      if (tag->val!=6) {
        fprintf(stderr,"TIFFTAG_VAL_PHOTOMETRICINTERPRETATION, only YCbCr (6) is supported\n");
        return -1;
      }
      break;
    case TIFFTAG_VAL_STRIPOFFSET:
      stripOffset = tag->val;
      break;
    case MAKERNOTE_MODEL_ID:
      cr2->modelId = tag->val;
      break;
    case MAKERNOTE_FIRMWARE_VERSION:
      fseek(in, tag->val, SEEK_SET);
      len = MIN(tag->len, CR2_FIRMWARE_LEN);
      fread(cr2->firmware, sizeof(char), len, in); 
      cr2->firmware[len]=0; // copy this in craw2.c
      break;
    case TIFFTAG_VAL_ORIGINAL_HSFVSF:
      cr2->jpeg.hsf = tag->val>>16;
      cr2->jpeg.vsf = tag->val & 0xffff;
      break;
    case TIFFTAG_VAL_STRIPBYTECOUNTS:
      stripBytesCounts = tag->val;
      break;
    case TIFFTAG_VAL_SAMPLESPERPIXEL:
      if (tag->val!=3) {
        fprintf(stderr,"TIFFTAG_VAL_SAMPLESPERPIXEL must be 3\n");
        return -1;
      }
      cr2->jpeg.ncomp = (unsigned char)tag->val;
      break;
    case TIFFTAG_VAL_YCBCRSUBSAMPLING:
      subSampling = tag->val;
      /*  case 0x00010004: //411, packed
       *  case 0x00010002: //422, packed
       *  case 0x00010001: //444
       */  
      break;
    default:
      ;    
    } // switch
    tag = tag->next;
    //printf("tag->val=%lx\n", tag->val);
  } // while

  fseek(in, stripOffset, SEEK_SET);
  if (subSampling!=0x00010001) {
    yuvData = malloc(stripBytesCounts);
    if (!yuvData) {
      fprintf(stderr,"yuvTiffOpen: malloc\n");
      return -1;
    }
    fread( yuvData, sizeof(short), stripBytesCounts/2, in); // endian
    //printf("unpack\n");
    // unpack yuvData into cr2->unsliced
    cr2->unsliced=(unsigned short*)malloc(cr2->jpeg.wide*cr2->jpeg.high*cr2->jpeg.ncomp * sizeof(short));
    if (!cr2->unsliced) {
      fprintf(stderr,"yuvTiffOpen: malloc unsliced\n");
      return -1;
    }
    cr2->unslicedLen = cr2->jpeg.wide*cr2->jpeg.high*cr2->jpeg.ncomp;
    yuvUnPack( cr2, yuvData, &unPackedLen);
  
    free(yuvData);
  }
  else {
    cr2->unsliced = (unsigned short*)malloc(stripBytesCounts);
    fread( cr2->unsliced, sizeof(short), stripBytesCounts/2, in); // endian
  }  
  fclose(in);
  return 0;
}

void makernote(FILE* in, unsigned long offset, unsigned long base, struct tiff_tag **lastTag)
{
  unsigned short n;
  int i;
  unsigned long savedfp;

  savedfp = ftell(in);
  fseek(in, offset+base, SEEK_SET);
  
  n = fiGet16(in);

  if (cr2Verbose>1)
    printf("    Makernote nEntries=%d\n", n);

  for (i=0; i<n; i++) {
    *lastTag = newTiffTag(*lastTag); // allocate memory and add tag cell to the linked list
    getEntry(in, *lastTag); // fill the tag with file content
    (*lastTag)->context = TIFFTAG_VAL_MAKERNOTE;
  }
  
  fseek(in, savedfp, SEEK_SET);
}


void exif(FILE* in, unsigned long offset, unsigned long base, struct tiff_tag **lastTag)
{
  short n;
  int i;
  unsigned long savedfp; 
 
  savedfp = ftell(in);
  fseek(in, offset+base, SEEK_SET);
  
  n = fiGet16(in);
  if (cr2Verbose>1)
    printf("  EXIF nEntries=%d\n", n);

  for (i=0; i<n; i++) {
	  *lastTag = newTiffTag(*lastTag); // allocate memory and add tag cell to the linked list
    getEntry(in, *lastTag); // fill the tag with file content
    (*lastTag)->context = TIFFTAG_VAL_EXIF;

    if ((*lastTag)->id==TIFFTAG_VAL_MAKERNOTE) {
      makernote(in, (*lastTag)->val, base, lastTag);
      if (cr2Verbose>1) 
        printf("    end of Makenotes\n");
    }

  }
  fseek(in, savedfp, SEEK_SET);
}

unsigned long parseIFD(FILE *in, unsigned long base, unsigned short ifd, struct tiff_tag **lastTag, struct tiff_tag **list)
{
  short n;
  int i;
  unsigned short context = ifd;
	
  n = fiGet16(in);
  if (cr2Verbose>1)
    printf("nEntries=%d\n", n);

  for (i=0; i<n; i++) {
	  *lastTag = newTiffTag(*lastTag); // allocate memory and add tag cell to the linked list
    if (*list==0)
		  *list = *lastTag; // root of the list
		
    getEntry(in, *lastTag); // fill the tag with file content
    (*lastTag)->context = context; // makernote, exif, ifd...

    if ((*lastTag)->id==TIFFTAG_VAL_EXIF) { 
      exif(in, (*lastTag)->val, base, lastTag);
      if (cr2Verbose>1)
        printf("  end of Exif\n");
    }
  }
  return ((unsigned long)fiGet32(in)); // next IFD
}


struct tiff_tag * parseTiff(FILE *in, unsigned long base)
{
  int nIfd;
  unsigned long offset, firstIFD, fileSize;
  struct tiff_tag *tagList, *lastTag;
  
  fseek(in, 0, SEEK_END);
  fseek(in, 0, SEEK_END);
  fileSize = ftell(in);

  fseek(in, 4, SEEK_SET);
  firstIFD = fiGet32(in); 	
  fseek(in, firstIFD, SEEK_SET);

  if (cr2Verbose>1)
    printf("firstIFD=0x%lx\n", firstIFD);
  if (firstIFD>fileSize) {
    fprintf(stderr,"parseTiff: firstIFD>fileSize\n");
    return 0;
  }
    
  nIfd = 0;
  tagList = lastTag = 0;
  do {
    if (cr2Verbose>1)
      printf("IFD #%d, ", nIfd);
 
	offset = parseIFD(in, base, nIfd, &lastTag, &tagList ); 
		
    if (offset) {
      if (cr2Verbose>1)
        printf("next IFD offset=0x%lx\n", offset+base);
      fseek(in, base+offset, SEEK_SET);
    }
    nIfd++;
  }while (offset);
	
	return tagList;
}  

void displayTagList(struct tiff_tag *list, FILE* in) {
  if (list!=0) {
    displayTag(list, in, 1);
		displayTagList(list->next, in);
	}
}

void displayTagListCsv(struct tiff_tag *list, FILE* in, int follow, int max) {
  if (list!=0) {
    displayTagCsv(list, in, follow, max);
		displayTagListCsv(list->next, in, follow, max);
	}
}

void freeTagList(struct tiff_tag *list) {
  if (list!=0) {
    if (list->next!=0)
		  freeTagList(list->next);
		free(list);	
	}
}

#ifndef CR2_TIFF
#define CR2_TIFF 1

#include"rgb.h"

#define TIFFTAG_VAL_MAKERNOTE                 0x927c //37500
#define TIFFTAG_VAL_EXIF                      0x8769 //34665
#define TIFFTAG_VAL_IMAGEWIDTH                0x0100
#define TIFFTAG_VAL_IMAGEHEIGHT               0x0101
#define TIFFTAG_VAL_BITSPERSAMPLES            0x0102
#define TIFFTAG_VAL_COMPRESSION               0x0103
#define TIFFTAG_VAL_PHOTOMETRICINTERPRETATION 0x0106
#define TIFFTAG_VAL_FILLORDER                 0x010A
#define TIFFTAG_VAL_STRIPOFFSET               0x0111 
#define TIFFTAG_VAL_SAMPLESPERPIXEL           0x0115 
#define TIFFTAG_VAL_ROWSPERSTRIP              0x0116 
#define TIFFTAG_VAL_STRIPBYTECOUNTS           0x0117 
#define TIFFTAG_VAL_XRESOLUTION               0x011a 
#define TIFFTAG_VAL_YRESOLUTION               0x011b 
#define TIFFTAG_VAL_PLANARCONFIGURATION       0x011C
#define TIFFTAG_VAL_RESOLUTIONUNIT            0x0128  	
#define TIFFTAG_VAL_SOFTWARE                  0x0131
#define TIFFTAG_VAL_WHITEPOINT                0x013e
#define TIFFTAG_VAL_PRIMARYCHROMACITIES       0x013f
#define TIFFTAG_VAL_YCBCRSUBSAMPLING          0x0212
#define TIFFTAG_VAL_YCBCRPOSITIONING          0x0213 
#define TIFFTAG_VAL_REFERENCEBLACKWHITE       0x0214
// libcraw2 custom
#define TIFFTAG_VAL_ORIGINAL_HSFVSF           0x4999
#define TIFFTAG_VAL_SRAW_WB_RATIO             0x4998
#define TIFFTAG_VAL_RAW_WB_RATIO              0x4997
 
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
#define TIFFTAG_TYPE_MAX TIFFTAG_TYPE_FLOAT8 

#define TIFFTAG_SUBIFD 0x014a

#define TIFFTAG_CONTEXTLEN 15

#include<stdio.h>

#include"export.h"

struct cr2file;

struct tiff_tag {
  unsigned short context;
  unsigned short id, type;
  unsigned long len;
  unsigned long val;
  unsigned long offset;
  struct tiff_tag *next;
};

void EXPORT displayTagList(struct tiff_tag *list, FILE *in);
void EXPORT displayTagListCsv(struct tiff_tag *list, FILE* in, int follow, int max);
void EXPORT freeTagList(struct tiff_tag *list);
int EXPORT export16BitsBufferAsTiff( char *filename, unsigned short *buffer, unsigned long h, unsigned long w);
int EXPORT exportYuvTiff(struct cr2file *cr2, char *filename, int interpolate, int packed);
int EXPORT exportRggbTiff(struct cr2file *cr2, char *filename);
int EXPORT exportRgbTiff(struct cr2image *image, char *filename, int g2_b_inversed);
int EXPORT yuvTiffOpen(struct cr2file *cr2, char *filename);
int EXPORT getTagTypeLen(int type);

struct tiff_tag EXPORT * parseTiff(FILE *in, unsigned long base);

#endif // CR2_TIFF


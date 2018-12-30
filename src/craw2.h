#ifndef CRAW2_H
#define CRAW2_H 1

#include<string.h>

#define CRAW2_VERSION_STR "0.8.0"

#ifndef MIN
#define MIN(a,b) ( ((a)<(b)) ? (a):(b) )
#endif
#ifndef MAX
#define MAX(a,b) ( ((a)>(b)) ? (a):(b) )
#endif
#ifndef ABS
#define ABS(a) ( ((a)>0) ? (a):-(a) )
#endif

// Canon TIFF constants
#define IFD3_STRIPOFFSETS 0x111
#define IFD3_STRIPBYTECOUNTS 0x117
#define IFD3_CR2SLICES 0xc640
#define IFD0_CAMERA_MODEL 272
#define MAKERNOTE_WHITE_BALANCE 0x4001
#define MAKERNOTE_MODEL_ID 0x10
#define MAKERNOTE_FIRMWARE_VERSION 7
#define MAKERNOTE_ASPECT_RATIO 0x9a
#define MAKERNOTE_SENSOR_INFO 0xe0
#define EXIF_IMAGE_WIDTH 0xa002
#define EXIF_IMAGE_HEIGHT 0xa003
	
#define LIBCRAW2_FILEVERSION 1
	
#define CR2_SLICES_LEN 3

#define CR2_HEADER_LENGTH 12

#define CR2_MODELSTRING_LEN 40
#define CR2_FIRMWARE_LEN 30

#define RAW_TYPE_RGGB 0
#define RAW_TYPE_YUV422 1
#define RAW_TYPE_YUV411 2

#include"export.h"

#include"lljpeg.h"
#include<stdio.h>
#include<stdint.h>


#ifndef fiPut16
//#define fiPut16(c,f) (fputc( ((uint16_t)c)&0xff,f)) ; (fputc(((((uint16_t)c)&0xff00)>>8),f)) 
//#define fiPut32(c,f) fiPut16(((c)&0xffff),f) ; fiPut16((((c&0xffff0000)>>16)),f)
#endif // fiPut16

#define LIBCRAW2_HEADER_SIZE 0x33

#define LIBCRAW2_FLAG_INTERPOLATED    1
#define LIBCRAW2_FLAG_DONOTUPDATE    -1

struct cr2file{
  // from TIFF tags
  unsigned long stripOffsets;
  unsigned long stripOffsetsB; // dual pixel
  unsigned long stripByteCounts;
  unsigned long stripByteCountsB; // dual pixel
  unsigned short cr2Slices[CR2_SLICES_LEN];
  unsigned short cr2SlicesB[CR2_SLICES_LEN]; // dual pixel
  unsigned short wbRggb[4];
  unsigned short srawRggb[4];
  unsigned short sensorWidth, sensorHeight; 
  unsigned short sensorLeftBorder, sensorTopBorder, sensorRightBorder, sensorBottomBorder;	
  unsigned short blackMaskLeftBorder, blackMaskTopBorder, blackMaskRightBorder, blackMaskBottomBorder;	
  unsigned short croppedImageWidth, croppedImageHeight;
  unsigned short exifImageWidth, exifImageHeight;
  unsigned long modelId;
  unsigned short filter;
  char *model;
  char firmware[CR2_FIRMWARE_LEN];
  FILE* file;
  struct tiff_tag *tagList;
  struct lljpeg jpeg;
  struct lljpeg jpegB; // dual pixel
  unsigned short *unsliced; 
  unsigned short *unslicedB;   // dual pixel
  unsigned long unslicedLen;
  unsigned char *yuvMask;
  int type; // RGGB, Y4UV or Y2UV
  unsigned long interpolated;
  // after verifying if vertical shifting is required
  unsigned short tBorder, bBorder, lBorder, rBorder;
  unsigned short vShift;
  unsigned short width, height;
  unsigned long fileSize;
};

uint16_t EXPORT fiGet16(FILE *f);
uint32_t EXPORT fiGet32(FILE *f);
void EXPORT fiPut16(uint16_t c, FILE *f);
void EXPORT fiPut32(uint32_t c, FILE *f);

EXPORT void cRaw2New(struct cr2file *cr2);
EXPORT void  cRaw2PrintTags( struct cr2file *cr2Data);
EXPORT int  cRaw2Open(struct cr2file *cr2, char *name);
EXPORT void  cRaw2Close(struct cr2file *cr2);
EXPORT int  cRaw2GetTags(struct cr2file *cr2);
EXPORT int  cRaw2Unslice(struct cr2file *cr2 );
EXPORT int  cRaw2Slice( struct cr2file *cr2, unsigned short *sliced);
EXPORT int  cRaw2PatchAs(char* filename, struct cr2file *cr2, unsigned char *compressed, unsigned long compLen, unsigned char* dht);

EXPORT char  *  cRaw2Info(struct cr2file *cr2Data);
EXPORT void  setVerboseLevel(int l);
EXPORT void  cRaw2SetType( struct cr2file *cr2Data);
EXPORT int  createFileTextHeader(struct cr2file *cr2, char *textHeader, unsigned long flags);
EXPORT unsigned long  computeYuvPackedSize(unsigned short wide, unsigned short high, int hsf, int vsf);
EXPORT int  cr2Import(char *filename);
EXPORT int  cRaw2FindImageBorders(struct cr2file *cr2, unsigned short *corners);
EXPORT void  cRaw2GetWbValues(struct cr2file *cr2, struct tiff_tag *tag );
EXPORT int  cRaw2GetTagValue( struct tiff_tag *tag, unsigned char **buf, FILE* file );
EXPORT int  hasDualPixel(struct cr2file *cr2);

extern int cr2Verbose;

//static char *rawTypeStr[] = { "RGGB", "YUV422/sraw/sraw2", "YUV411/mraw/sraw1" };

#endif // CRAW2_H 

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
  
  unsigned char *scandata;
  unsigned long compressedLen, col, line;
  struct cr2image image, image2;
  int quiet=0;
  int pixelVerbose = -10000; // disabled

  char *patchImage=0;
  
  char tagContext=0;
  unsigned short tagId;
  char tagAccess;

  if (argc>1) {
    if (cRaw2Open(&cr2, argv[1] )<0) 
      exit(1);
	}
	else {
	  usage();
    exit(1);
  }
  // retrieve important TIFF tags for RAW processing
  if ( cRaw2GetTags( &cr2)<0 )
	  exit(1);
 
  if (jpegParseHeaders( cr2.file, cr2.stripOffsets, &(cr2.jpeg) )<0)
    exit(1);  

  puts( cRaw2Info(&cr2) );
    
  // decompress from file to jpeg.rawBuffer
  jpegDecompress( &(cr2.jpeg), cr2.file, cr2.stripOffsets, cr2.stripByteCounts ) ;

  // unslice from jpeg.rawBuffer to unsliced
  cRaw2Unslice( &cr2 );


  if (cr2.jpeg.hsf==1) { // for RAW only

    printf("this file is RGGB, not YUV !\n");
  
  } // RAW
  else { // YUV
    struct cr2file cr2b;

    if (!cr2.yuvMask)
      yuvMaskCreate(&cr2); // create a mask array where 1 means "existing value", 0 "missing value"
    //exportFile(outfile, "_mask.bin", cr2.yuvMask, (cr2.jpeg.wide*cr2.jpeg.ncomp*cr2.jpeg.high)/2, 1); // size in short

    //yuvInterpolate(&cr2); 
    
    exportYuvTiff(&cr2, "yuv16.tiff", 0, 0); 
    exportYuvTiff(&cr2, "yuv16i.tiff", 1, 0); // interpolate before saving, unsliced is modified
    exportYuvTiff(&cr2, "yuv16p.tiff", 0, 1); // pack, unsliced not modified, but interpolated data are removed by packing 
    cr2ImageNew(&image);
    image.isRGGB = 0; // YCbCr and not RGGB
    yuv2rgb(&cr2, &image); // conversion to RGB and WB correction (dcraw method)
    exportRgbTiff(&image, "yuv2rgb.tiff", 0);
    cr2ImageFree(&image);
        
    // reload packed YUV data
    yuvTiffOpen(&cr2b, "yuv16p.tiff");
    
    // all required data is here for interpolation and YUV 2 RGB conversion
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
    exportYuvTiff(&cr2, "yuv16b.tiff", 0, 0); 
    
    yuvInterpolate(&cr2b); 
    exportYuvTiff(&cr2, "yuv16ib.tiff", 0, 0); 
    
    cr2ImageNew(&image2);
    image.isRGGB = 0; // YCbCr and not RGGB
    
    yuvInterpolate(&cr2);
    yuv2rgb(&cr2, &image2); // conversion to RGB and WB correction (dcraw method)

    exportRgbTiff(&image2, "yuv2rgb2.tiff", 0);

    cr2ImageFree(&image2);
    cRaw2Close( &cr2b );  

    cr2ImageFree(&image);
    
  } // end of YUV

  cRaw2Close( &cr2 );  
	
  return(0);
}

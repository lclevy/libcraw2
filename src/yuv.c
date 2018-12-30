/*
 * YCbCr related functions
 */

#include"yuv.h"
#include"craw2.h"
#include"rgb.h"
#include<ctype.h> // isdigit()
#include<stdio.h>

/*
 * interpolate missing UV information for Y4UV and Y2UV in unsliced[]
 *
 * Y4UV format before interpolation:
 *   YUV Y.. YUV Y..
 *   Y.. Y.. Y.. Y.. <-- first interpolation (Y4UV only). After this pass, Y4UV is like Y2UV
 * Y2UV:
 *   YUV Y.. YUV Y.. 
 *   YUV Y.. YUV Y..
 * "." means missing value 
 *
 * U and V are signed values! 
 * output is identical to DCraw before YUV->RGB conversion, except 5D Mark II and 50D mraw, because Dcraw remove some columns from right border.
 */
int yuvInterpolate( struct cr2file *cr2 ) {
  long line, column;
  long unslicedRowSize, destBaseOffset;
  int comp;
  short *signed_image;

  signed_image = (short*)(cr2->unsliced);

  if (cr2->unsliced==0) {
    fprintf(stderr, "yuvInterpolate: no unsliced data!\n");
    return -1;
  }		
  if (cr2->jpeg.hsf==1) {
    fprintf(stderr, "yuvInterpolate: can not interpolate RGGB data\n");
    return -1;
  }		
  unslicedRowSize = cr2->jpeg.wide * cr2->jpeg.ncomp;
	//puts("yuvInterpolate");
	cr2->interpolated = 1;
	
  // Y4UV: no UV info on even lines, we must interpolate it first for odd columns
  if (cr2->jpeg.vsf==2) { 
    for (line=1; line < cr2->jpeg.high-1; line+=2) { // well line#1 is the second one, thus called "even"
      for (column=0; column<cr2->jpeg.wide; column+=2) {
        destBaseOffset = line*unslicedRowSize + column*cr2->jpeg.ncomp;
        for(comp=1; comp<3; comp++) // average UV values from previous and next lines
          signed_image[ destBaseOffset+comp ] = ( signed_image[ destBaseOffset-unslicedRowSize+comp ] + signed_image[ destBaseOffset+unslicedRowSize+comp ] + 1 ) >> 1;
      }
    }
    // last line can not be interpolated the same way
    line = cr2->jpeg.high-1; 
    for (column=0; column<cr2->jpeg.wide; column+=2) {
      destBaseOffset = line*unslicedRowSize + column*cr2->jpeg.ncomp;
      for(comp=1; comp<3; comp++) 
        signed_image[ destBaseOffset+comp ] = signed_image[ destBaseOffset-unslicedRowSize+comp ];  // just copy values from previous line
    }
  } // vsf==2
	
  // UV interpolation for even columns
  for (line=0; line < cr2->jpeg.high; line++) { 
    for (column=1; column<cr2->jpeg.wide-1; column+=2) {
      destBaseOffset = line*unslicedRowSize + column*cr2->jpeg.ncomp;
      for(comp=1; comp<3; comp++) // average UV values from previous and next columns
        signed_image[ destBaseOffset+comp ] = ( signed_image[ destBaseOffset+comp - cr2->jpeg.ncomp ] + signed_image[ destBaseOffset+comp + cr2->jpeg.ncomp ] + 1 ) >> 1;
    }
  }
  // last column can not be interpolated the same way
  column = cr2->jpeg.wide-1; 
  for (line=0; line < cr2->jpeg.high; line++) { 
    destBaseOffset = line*unslicedRowSize + column*cr2->jpeg.ncomp;
    for(comp=1; comp<3; comp++) 
      signed_image[ destBaseOffset+comp ] = signed_image[ destBaseOffset+comp - cr2->jpeg.ncomp ]; // UV values from previous column
  }
  return 0;	
}
/*
unsigned long computeYuvPackedSize(unsigned short wide, unsigned short high, int hsf, int vsf) {
  return wide*high + ( (2*wide*high) / (hsf*vsf) );
}
*/
/*
 * packs an unsliced image (suppress empty pixels)
 * 
 * data written in packed, length written is packedLen (in short)
 *
 * packedLen should be  cr2->high * cr2->jpeg.wide * cr2->jpeg.ncomp - (cr2->high * cr2->jpeg.wide * (hsf-1))
 */
int yuvPack( struct cr2file *cr2, unsigned short *packed, unsigned long *packedLen )
{
  long line, column;
	int comp;
  long unslicedRowSize, i=0;
	
	if (!cr2)
	  return -1;
	if (!cr2->unsliced)
    return -1;	
	
  unslicedRowSize = cr2->jpeg.wide * cr2->jpeg.ncomp;
	
  for(line=0; line<cr2->jpeg.high; line++)  {
    for(column=0; column<cr2->jpeg.wide; column+=cr2->jpeg.hsf) {
      if (cr2->jpeg.vsf!=2 || (line%2)==0)  // lines with YUV Y..
        for (comp=0; comp<cr2->jpeg.ncomp; comp++)  // YUV
		  	  packed[i++] = cr2->unsliced[line*unslicedRowSize + column*cr2->jpeg.ncomp + comp];
      else   // lines with Y.. Y..
        packed[i++] = cr2->unsliced[line*unslicedRowSize + column*cr2->jpeg.ncomp]; // Y..
			
      packed[i++] = cr2->unsliced[line*unslicedRowSize + (column+1)*cr2->jpeg.ncomp]; // next column: Y..
    } // column	
	}	
	*packedLen = i;	

	return 0; // OK
}

/* 
 * read in packed and write in unsliced
 * clear cr2->interpolated
 */
int yuvUnPack( struct cr2file *cr2, unsigned short *packed, unsigned long *unPackedLen )
{
  long line, column;
	int comp;
  unsigned long unslicedRowSize, i=0;
	
	if (!cr2)
	  return -1;
	if (!cr2->unsliced)
    return -1;	
	
  unslicedRowSize = cr2->jpeg.wide * cr2->jpeg.ncomp;
	
  for(line=0; line<cr2->jpeg.high; line++)  {
    for(column=0; column<cr2->jpeg.wide; column+=cr2->jpeg.hsf) {
      if (cr2->jpeg.vsf!=2 || (line%2)==0)  // lines with YUV Y..
        for (comp=0; comp<cr2->jpeg.ncomp; comp++)  // YUV
		  	  cr2->unsliced[line*unslicedRowSize + column*cr2->jpeg.ncomp + comp] = packed[i++];
      else   // lines with Y.. Y..
        cr2->unsliced[line*unslicedRowSize + column*cr2->jpeg.ncomp] = packed[i++]; // Y..
			
      cr2->unsliced[line*unslicedRowSize + (column+1)*cr2->jpeg.ncomp] = packed[i++]; // next column: Y..
    } // column	
	}	
	*unPackedLen = i;	

	return 0; // OK
}

/*
 * YCbCr (for sraw data) conversion to RGB
 *
 * using Dcraw / Canon implementations (DPP v3, dppdpp.dll, look for constants)
 *
 * needs cr2->modelId, cr2->firmware, cr2->jpeg.vsf, cr2->jpeg.wide, cr2->jpeg.high, cr2->srawRggb[]
 */
int yuv2rgb( struct cr2file *cr2, struct cr2image *img ) {
  char *cp; // char pointer (on firmware version)
  int v[3]={0,0,0}, ver, hue;
  short *rp, *outp; // raw pointer
  int isDigic4 = 0;  
  int blackSub, dcsraw;
  int pix[3];

  if (cr2->jpeg.bits!=15) {
    return -1;
  }
  if (cr2->jpeg.vsf==2) //sraw1
    dcsraw = 3;
  else // sraw2
    dcsraw = 1;  

  // code from dcraw (I added comments ;-)
  for (cp=cr2->firmware; *cp && !isdigit(*cp); cp++); // find the first digit occurrence in firmware version text
  sscanf (cp, "%d.%d.%d", v, v+1, v+2);
  ver = (v[0]*1000 + v[1])*1000 + v[2]; // conversion firmware version in into integer for easier comparison
  //printf("ver=%d\n", ver);
    
  hue = (dcsraw+1) << 2;
  // 5dm2 update 1.0.7 : http://cpn.canon-europe.com/content/news/eos_5d_mk_2_firmware_107.do
  // I suppose after this update for "Vertical banding noise" correction (sraw1), it requires a different "hue" value
  if (cr2->modelId >= 0x80000281 || (cr2->modelId == 0x80000218 && ver > 1000006))  // after 1Dm4 or 5Dm2 updated at least with firmware 1.0.7
    hue = dcsraw << 1;
  //printf("hue=%d jh.sraw=%d\n", hue, dcsraw);

  img->width = cr2->jpeg.wide;
  img->height = cr2->jpeg.high ; 
  img->pixels = malloc( img->width * img->height *3 * sizeof(short));
  if (!img->pixels) {
    fprintf(stderr, "yuv2rgb: malloc\n");
    return -1;
  }

  isDigic4 = cr2->modelId == 0x80000218 || 	cr2->modelId == 0x80000250 || 	cr2->modelId == 0x80000261 || 	cr2->modelId == 0x80000281 || 	cr2->modelId == 0x80000287;
  if (cr2->modelId < 0x80000218) // before 5Dm2
    blackSub = 512; // I suppose it is black subtraction
  else  
    blackSub = 0;

  outp = (short*)img->pixels;  
  for ( rp=(short*)cr2->unsliced; rp < (short*)cr2->unsliced + cr2->unslicedLen; rp+=3) {
    /*for(c=0; c<3; c++)
      printf("%d ", rp[c]);
    printf("\n");  */
    
    if (isDigic4) {
      rp[1] = (rp[1] << 2) + hue;
      rp[2] = (rp[2] << 2) + hue;
      pix[0] = rp[0] + ((   50*rp[1] + 22929*rp[2]) >> 14);  
      pix[1] = rp[0] + ((-5640*rp[1] - 11751*rp[2]) >> 14);
      pix[2] = rp[0] + ((29040*rp[1] -   101*rp[2]) >> 14);
      /*
       */
    } else { // Digic3 and Digic > 4
      pix[0] = rp[0] + rp[2] - blackSub;
      pix[2] = rp[0] + rp[1] - blackSub;
      pix[1] = rp[0] + ((-778*rp[1] - (rp[2] << 11)) >> 12) - blackSub; 
      /* -778/(2^12) = 0.19, (2^11)/(2^12)=0.5. pix[1] = rp[0] - 0.19 rp[1] - 0.5 rp[2] - 512
       * if rp[0] = Y, rp[1] = 2Cb, rp[2] = 1.6Cr (Gao YANG, dec 2007, http://www.dpreview.com/forums/post/26234685)
       *  R' = Y + 1.6 Cr, B' = Y + 2 Cb, G' = Y - 0.38 Cb - 0.8 Cr   
       *  R = Y + 1.6 Cr - 512, 
       *  B = Y + 2 Cb - 512, 
       *  G = Y - 0.38 Cb - 0.8 Cr - 512
       * from JFIF specification, page 3:
       *  RGB can be computed directly from YCbCr (256 levels) as follows:
       *  R = Y + 1.402 Cr
       *  G = Y - 0.34414 Cb - 0.71414 Cr
       *  B = Y + 1.772 Cb
       */
    }
    /*for(c=0; c<3; c++)
      printf("%d ", pix[c]);
    printf("-\n");  */
    
    // white balance adjustment
    outp[0] = MAX(0, MIN( pix[0] * cr2->srawRggb[0] >> 10, 65535)); // red
    outp[1] = MAX(0, MIN( pix[1] * cr2->srawRggb[1] >> 10, 65535)); // green
    outp[2] = MAX(0, MIN( pix[2] * cr2->srawRggb[3] >> 10, 65535)); // blue
    
   /*for(c=0; c<3; c++)
      printf("%d ", outp[c]);
    printf("#\n");  */
    outp+=3;
  }
  return 0;
}

/*
 * fills a 1D array which represents a 2D YUV mask where 1 means 'value from CR2' and 0 means 'missing value' for Python matrix operations
 * must be allocated and filled with 0 before:
 *  calloc( cr2->jpeg.wide * cr2->jpeg.ncomp * cr2->jpeg.high * sizeof(char) )
 */
int yuvMaskCreate(struct cr2file *cr2) {
  unsigned long unslicedRowSize;
  unsigned short line, column ;
  int  comp ;
  
  unslicedRowSize = cr2->jpeg.wide * cr2->jpeg.ncomp;
  cr2->yuvMask = calloc( unslicedRowSize * cr2->jpeg.high, sizeof(char)  );
  if (!cr2->yuvMask) {
    fprintf(stderr,"yuvMask");
    return -1;
  }
  for(line=0; line<cr2->jpeg.high; line++)  {
    for(column=0; column<cr2->jpeg.wide; column+=cr2->jpeg.hsf) { // cr2->jpeg.hsf always ==2
      if (cr2->jpeg.vsf!=2 || (line%2)==0)  // lines with YUV Y..
        for (comp=0; comp<cr2->jpeg.ncomp; comp++)  // YUV
		  	  cr2->yuvMask[line*unslicedRowSize + column*cr2->jpeg.ncomp + comp] = 1;
      else   // lines with Y.. Y..
        cr2->yuvMask[line*unslicedRowSize + column*cr2->jpeg.ncomp] = 1; // Y..
			
      cr2->yuvMask[line*unslicedRowSize + (column+1)*cr2->jpeg.ncomp] = 1; // next column: Y..
    } // column	
	}	
  return 0;
}

char *colorStrYuv[NB_COLORS_YUV]={ "Y", "Cb", "Cr" };

int yuvComputeImageStats(struct cr2file *cr2) {

  long black[NB_COLORS_YUV], white[NB_COLORS_YUV] ;
  unsigned long counts[NB_COLORS_YUV], unslicedRowSize;
  double sums[NB_COLORS_YUV], stdev[NB_COLORS_YUV], diff;
  short compValue;
  unsigned short line, column ;
  int i, comp ;
	
	for(i=0; i<NB_COLORS_YUV; i++) {
	  sums[i] = stdev[i] = 0;
		black[i] = 32000;
		white[i] = -32000;
		counts[i] = 0;
  }	
  /* count only original values, not empty / interpolated ones
   * we can have
   * YUV Y.. (hsf=2, vsf=1)
   * YUV Y..
   * or
   * YUV Y.. (hsf=2, vsf=2)
   * Y.. Y..
   */
  unslicedRowSize = cr2->jpeg.wide * cr2->jpeg.ncomp;
	for(line=0; line<cr2->jpeg.high; line++) {
      //printf("line=%d ", line);
	  for(column=0; column<cr2->jpeg.wide; column+=cr2->jpeg.hsf) { // cr2->jpeg.hsf always == 2 with YCbCr data
      //printf("column=%d: ", column);
      if (cr2->jpeg.vsf!=2 || (line%2)==0)  // lines with YUV Y..
        for (comp=0; comp<cr2->jpeg.ncomp; comp++)  { // YUV
          compValue = cr2->unsliced[line*unslicedRowSize + column*cr2->jpeg.ncomp + comp];
          //printf("%d %x. ", compValue, compValue);
			    black[comp] = MIN( compValue, black[comp] );	
		      white[comp] = MAX( compValue, white[comp] );	
          counts[comp]++;
          sums[comp]+= compValue;
        }  // YUV
      else {  // lines with Y.. Y..
        compValue = cr2->unsliced[line*unslicedRowSize + column*cr2->jpeg.ncomp]; // Y..
        black[0] = MIN( compValue, black[0] );	
	      white[0] = MAX( compValue, white[0] );	
        counts[0]++;
        sums[0]+= compValue;
      }			
      compValue =  cr2->unsliced[line*unslicedRowSize + (column+1)*cr2->jpeg.ncomp]; // next column: Y.. (cr2->jpeg.hsf always == 2)
          //printf("%d ", compValue);
	    black[0] = MIN( compValue, black[0] );	
      white[0] = MAX( compValue, white[0] );	
      counts[0]++;
      sums[0]+= compValue;
          //printf(", ");
    }
  }
  // computes standard deviation
  for(line=0; line<cr2->jpeg.high; line++)
    for(column=0; column<cr2->jpeg.wide; column+=cr2->jpeg.hsf) { // cr2->jpeg.hsf always == 2 with YCbCr data
      if (cr2->jpeg.vsf!=2 || (line%2)==0)  // lines with YUV Y..
        for (comp=0; comp<cr2->jpeg.ncomp; comp++)  { // YUV
          compValue = cr2->unsliced[line*unslicedRowSize + column*cr2->jpeg.ncomp + comp];
          diff = compValue - (sums[comp]/counts[comp]); // pixel - mean
          stdev[comp] += (diff*diff);
        }  // YUV
      else {  // lines with Y.. Y..
        compValue = cr2->unsliced[line*unslicedRowSize + column*cr2->jpeg.ncomp]; // Y..
        diff = compValue - (sums[0]/counts[0]); // pixel - mean
        stdev[0] += (diff*diff);
      }
      compValue =  cr2->unsliced[line*unslicedRowSize + (column+1)*cr2->jpeg.ncomp]; // next column: Y.. (cr2->jpeg.hsf always == 2)
        diff = compValue - (sums[0]/counts[0]); // pixel - mean
        stdev[0] += (diff*diff);
    }	
   
	for(i=0; i<NB_COLORS_YUV; i++)
    stdev[i]	= sqrt( stdev[i]/counts[i] );

  printf("   ");
  for(i=0; i<NB_COLORS_YUV; i++)
    printf("%15s ", colorStrYuv[ i ] );	
  putchar('\n'); 
	
  printf("black =");
	for(i=0; i<NB_COLORS_YUV; i++)
	  printf("%15ld ", black[i]);
	printf("\nwhite =");
	for(i=0; i<NB_COLORS_YUV; i++)
	  printf("%15ld ", white[i]);
	printf("\ncounts=");
	for(i=0; i<NB_COLORS_YUV; i++)
	  printf("%15ld ", counts[i]);
	printf("\nsums  =");
	for(i=0; i<NB_COLORS_YUV; i++)
	  printf("%15.0f ", sums[i]);
	printf("\navg   =");
	for(i=0; i<NB_COLORS_YUV; i++)
	  if (sums[i]!=0) printf("%15.4f ", sums[i]/counts[i]);
	printf("\nstdev =");
  for(i=0; i<NB_COLORS_YUV; i++)
    printf("%15.4f ", stdev[i]);
  printf("\nEV    =");	
  for(i=0; i<NB_COLORS_YUV; i++)
    printf("%15.4f ", Log2( (double)white[i] - (double)black[i] ) ); 
  printf("\n");	

	return 0;
}

// rgb.c
// -*- tab-width:2; -*-
#include"craw2.h"
#include"rgb.h"
#include<math.h>

extern int cr2Verbose; // in craw2.c
//int imageStats=0;

double Log2( double n )  
{  
    // log(n)/log(2) is log2.  
    return log( n ) / log( 2.0 );  
}

char *colorStr[NB_COLORS]={ "r", "g1", "g2", "b" };

/*
 * image init
 * RGGB by default (tested in exportRgbTIFF() )
 */
int cr2ImageNew(struct cr2image *image) {
  image->isRGGB = 1;
  image->pixels = 0;
  return 0;
}

int cr2ImageFree(struct cr2image *image) {
  if (image->pixels)
    free(image->pixels);
  return 0;  
}

int computeImageStats(struct cr2image *image) {

  unsigned short black[NB_COLORS], white[NB_COLORS];
  unsigned long counts[NB_COLORS];
  double sums[NB_COLORS], stdev[NB_COLORS], diff;
  unsigned short line, column, color;
  int i;
	
  for(i=0; i<NB_COLORS; i++) {
    sums[i] = stdev[i] = 0;
    black[i] = 32000;
    white[i] = 0;
    counts[i] = 0;
  }	

  for(line=0; line<image->height; line++)
    for(column=0; column<image->width; column++) {
      color = FILTER_COLOR(line, column);
      black[color] = MIN( IMAGE_PIXEL(line,column,color), black[color] );	
      white[color] = MAX( IMAGE_PIXEL(line,column,color), white[color] );	
      counts[color]++;
      sums[color]+= IMAGE_PIXEL(line,column,color);
    }

  // computes standard deviation
  for(line = 0; line <image->height; line++) 
    for(column = 0; column <image->width; column++) {
      color = FILTER_COLOR(line, column);
      diff = IMAGE_PIXEL(line,column,color) - (sums[color]/counts[color]); // pixel - mean
      stdev[color] += (diff*diff);
    }
  for(i=0; i<NB_COLORS; i++)
    stdev[i] = sqrt( stdev[i]/counts[i] );

  printf("   ");
  for(line=0; line<2; line++) 
    for(column=0; column<2; column++) 
      printf("%15s ", colorStr[ FILTER_COLOR(line, column) ] );	
  putchar('\n'); 
	
  printf("black =");
	for(i=0; i<NB_COLORS; i++)
	  printf("%15d ", black[i]);
	printf("\nwhite =");
	for(i=0; i<NB_COLORS; i++)
	  printf("%15d ", white[i]);
	printf("\ncounts=");
	for(i=0; i<NB_COLORS; i++)
	  printf("%15ld ", counts[i]);
	printf("\nsums  =");
	for(i=0; i<NB_COLORS; i++)
	  printf("%15.0f ", sums[i]);
	printf("\navg   =");
	for(i=0; i<NB_COLORS; i++)
	  if (sums[i]!=0) printf("%15.4f ", sums[i]/counts[i]);
	printf("\nstdev =");
  for(i=0; i<NB_COLORS; i++)
    printf("%15.4f ", stdev[i]);
  printf("\nEV    =");	
  for(i=0; i<NB_COLORS; i++)
    printf("%15.4f ", Log2( (double)white[i] - (double)black[i] ) ); 
  printf("\n");	

	return 0;
}



int dumpImageCorners(struct cr2image *image) {
  unsigned short line, column, color;
	
	for(line = 0; line < 4; line++) {
	  for(column = 0; column <4; column++) {
 	    for(color = 0;  color<NB_COLORS; color++) 
				printf("%5u", image->pixels[ PIXEL_INDEX(line,column,color) ] );
      putchar('|'); 
		}	
    putchar('\n'); 
	}
  putchar('\n'); 
	for(line = 0 ; line < 4; line++) {
	  for(column = image->width-4; column < image->width; column++) {
 	    for(color = 0;  color<NB_COLORS; color++) 
				printf("%5u", image->pixels[ PIXEL_INDEX(line,column,color) ] );
      putchar('|'); 
		}	
    putchar('\n'); 
	}
  putchar('\n'); 
	for(line = image->height-4; line < image->height; line++) {
	  for(column = 0; column <4; column++) {
 	    for(color = 0;  color<NB_COLORS; color++) 
				printf("%5u", image->pixels[ PIXEL_INDEX(line,column,color) ] );
      putchar('|'); 
		}	
    putchar('\n'); 
	}
  putchar('\n'); 
	for(line = image->height-4; line < image->height; line++) {
	  for(column = image->width-4; column < image->width; column++) {
 	    for(color = 0;  color<NB_COLORS; color++) 
				printf("%5u", image->pixels[ PIXEL_INDEX(line,column,color) ] );
      putchar('|'); 
		}	
    putchar('\n'); 
	}
  putchar('\n'); 
	return 0;

}

/*
 * image is supposed to be R G R G on lines 0, 2, 4 ...
 *                         G B G B in lines 1, 3, 5 ... 
 *                         R G R G
 *                         G B G B 
 * http://scien.stanford.edu/pages/labsite/2007/psych221/projects/07/Dargahi&Deshpande.pdf, pages 7 and 8
 * http://www.amazon.com/Digital-Handbook-Electrical-Engineering-Processing/dp/084930900X, chapter 12.4
 * U.S. Patent 4,642,678 (feb 1986)
 */
int interpolateRGB_bilinear(struct cr2image *image) {
  unsigned short line, column;

  /* G1 interpolation, use G1 and G2 values */
	image->pixels[ PIXEL_INDEX(0,0,GREEN1_VALUE) ] = ( IMAGE_PIXEL(1,0,GREEN2_VALUE) + IMAGE_PIXEL(0,1,GREEN1_VALUE) )/2; // top left corner (R)
	for(line=2; line<image->height; line+=2) // at R positions, center of a +
    for(column=2; column<image->width; column+=2)  
		  image->pixels[ PIXEL_INDEX(line,column,GREEN1_VALUE) ] = ( IMAGE_PIXEL(line,column-1,GREEN1_VALUE) + IMAGE_PIXEL(line,column+1,GREEN1_VALUE) 
			+ IMAGE_PIXEL(line-1,column,GREEN2_VALUE) + IMAGE_PIXEL(line+1,column,GREEN2_VALUE) )/4;
	for(line=2; line<image->height; line+=2)  // first column at R positions
		image->pixels[ PIXEL_INDEX(line,0,GREEN1_VALUE) ] = ( IMAGE_PIXEL(line-1,0,GREEN2_VALUE) + IMAGE_PIXEL(line+1,0,GREEN2_VALUE) + IMAGE_PIXEL(line,1,GREEN1_VALUE) )/3; 
  for(column=2; column<image->width; column+=2)  // first line at R positions
	  image->pixels[ PIXEL_INDEX(0,column,GREEN1_VALUE) ] = ( IMAGE_PIXEL(0,column-1,GREEN1_VALUE) + IMAGE_PIXEL(0,column+1,GREEN1_VALUE) + IMAGE_PIXEL(1,column,GREEN2_VALUE) )/3;
		
	image->pixels[ PIXEL_INDEX(image->height-1,image->width-1,GREEN1_VALUE) ] = ( IMAGE_PIXEL(image->height-2,image->width-1,GREEN1_VALUE) 
	  + IMAGE_PIXEL(image->height-1,image->width-2,GREEN2_VALUE) )/2;	// bottom right corner (B)
	for(line=1; line<image->height-1; line+=2) // at B positions, center of a +
    for(column=1; column<image->width-1; column+=2)  
		  image->pixels[ PIXEL_INDEX(line,column,GREEN1_VALUE) ] = ( IMAGE_PIXEL(line-1,column,GREEN1_VALUE) + IMAGE_PIXEL(line+1,column,GREEN1_VALUE) 
			+ IMAGE_PIXEL(line,column-1,GREEN2_VALUE) + IMAGE_PIXEL(line,column+1,GREEN2_VALUE) )/4; 
  for(column=1; column<image->width-1; column+=2)  // last line at B positions
	 image->pixels[ PIXEL_INDEX(image->height-1,column,GREEN1_VALUE) ] = ( IMAGE_PIXEL(image->height-2,column,GREEN1_VALUE)
	 + IMAGE_PIXEL(image->height-1,column-1,GREEN2_VALUE) + IMAGE_PIXEL(image->height-1,column+1,GREEN2_VALUE) )/3;
	for(line=1; line<image->height-1; line+=2)  // last column at B positions
		image->pixels[ PIXEL_INDEX(line,image->width-1,GREEN1_VALUE) ] = ( IMAGE_PIXEL(line-1,image->width-1,GREEN1_VALUE) 
		+ IMAGE_PIXEL(line+1,image->width-1,GREEN1_VALUE) + IMAGE_PIXEL(line,image->width-2,GREEN2_VALUE) )/3; 
  // copy G2 into G1
	for(line=1; line<image->height; line+=2) 
    for(column=0; column<image->width; column+=2)  
        image->pixels[ PIXEL_INDEX(line,column,GREEN1_VALUE) ] = IMAGE_PIXEL(line,column,GREEN2_VALUE);
  // copy G1 into G2
	for(line=0; line<image->height; line+=2) 
    for(column=1; column<image->width; column+=2)  
		image->pixels[ PIXEL_INDEX(line,column,GREEN2_VALUE) ] = IMAGE_PIXEL(line,column,GREEN1_VALUE);

	/* R interpolation */		
	for(line=1; line<image->height-1; line+=2) // at B positions, center of an X
    for(column=1; column<image->width-1; column+=2)  
		  image->pixels[ PIXEL_INDEX(line,column,RED_VALUE) ] = ( IMAGE_PIXEL(line-1,column-1,RED_VALUE) + IMAGE_PIXEL(line-1,column+1,RED_VALUE)
			+ IMAGE_PIXEL(line+1,column-1,RED_VALUE) + IMAGE_PIXEL(line+1,column+1,RED_VALUE) ) / 4 ;
  for(column=1; column<image->width-1; column+=2)  // last line at B positions
		image->pixels[ PIXEL_INDEX(image->height-1,column,RED_VALUE) ] = ( IMAGE_PIXEL(image->height-2,column-1,RED_VALUE) + IMAGE_PIXEL(image->height-2,column+1,RED_VALUE) )/ 2;
	for(line=1; line<image->height-1; line+=2)  // last column at B positions
		image->pixels[ PIXEL_INDEX(line,image->width-1,RED_VALUE) ] = ( IMAGE_PIXEL(line-1,image->width-2,RED_VALUE) + IMAGE_PIXEL(line+1,image->width-2,RED_VALUE) ) / 2; 
	image->pixels[ PIXEL_INDEX(image->height-1,image->width-1,RED_VALUE) ] = IMAGE_PIXEL(image->height-2,image->width-2,RED_VALUE); // bottom right corner (B)
		
	for(line=0; line<image->height; line+=2)  // last column at G positions
		image->pixels[ PIXEL_INDEX(line,image->width-1,RED_VALUE) ] = IMAGE_PIXEL(line,image->width-2,RED_VALUE); 
	for(line=1; line<image->height-1; line+=2) // at G positions, R values are on same column
    for(column=0; column<image->width; column+=2)  
		  image->pixels[ PIXEL_INDEX(line,column,RED_VALUE) ] = ( IMAGE_PIXEL(line-1,column,RED_VALUE) + IMAGE_PIXEL(line+1,column,RED_VALUE) ) / 2;
	for(line=0; line<image->height; line+=2) // at G positions, R values are on same line
    for(column=1; column<image->width-1; column+=2)  
		  image->pixels[ PIXEL_INDEX(line,column,RED_VALUE) ] = ( IMAGE_PIXEL(line,column-1,RED_VALUE) + IMAGE_PIXEL(line,column+1,RED_VALUE) ) / 2;
  for(column=0; column<image->width-1; column+=2)  // last line at G positions, R on top
		image->pixels[ PIXEL_INDEX(image->height-1,column,RED_VALUE) ] = IMAGE_PIXEL(image->height-2,column,RED_VALUE); 

	/* B interpolation */		
	for(line=2; line<image->height-1; line+=2) // at R positions, center of an X
    for(column=2; column<image->width-1; column+=2)  
		  image->pixels[ PIXEL_INDEX(line,column,BLUE_VALUE) ] = ( IMAGE_PIXEL(line-1,column-1,BLUE_VALUE) + IMAGE_PIXEL(line-1,column+1,BLUE_VALUE)
			+ IMAGE_PIXEL(line+1,column-1,BLUE_VALUE) + IMAGE_PIXEL(line+1,column+1,BLUE_VALUE) )  /4;
	for(line=2; line<image->height-1; line+=2) // at G positions, B values are on same column
    for(column=1; column<image->width; column+=2)  
		  image->pixels[ PIXEL_INDEX(line,column,BLUE_VALUE) ] = ( IMAGE_PIXEL(line-1,column,BLUE_VALUE) + IMAGE_PIXEL(line+1,column,BLUE_VALUE) ) /2;
	for(line=1; line<image->height; line+=2) // at G positions, B values are on same line
    for(column=2; column<image->width; column+=2)  
		  image->pixels[ PIXEL_INDEX(line,column,BLUE_VALUE) ] = ( IMAGE_PIXEL(line,column-1,BLUE_VALUE) + IMAGE_PIXEL(line,column+1,BLUE_VALUE) ) /2;
  for(column=1; column<image->width; column+=2)  // first line at G positions, B on bottom
 	  image->pixels[ PIXEL_INDEX(0,column,BLUE_VALUE) ] = IMAGE_PIXEL(1,column,BLUE_VALUE);
	for(line=1; line<image->height; line+=2)  // first column at G positions, B on right
 	  image->pixels[ PIXEL_INDEX(line,0,BLUE_VALUE) ] = IMAGE_PIXEL(line,1,BLUE_VALUE);
 	image->pixels[ PIXEL_INDEX(0,0,BLUE_VALUE) ] = IMAGE_PIXEL(1,1,BLUE_VALUE); // top left corner
  for(column=2; column<image->width; column+=2)  // first line at R positions
	  image->pixels[ PIXEL_INDEX(0,column,BLUE_VALUE) ] = ( IMAGE_PIXEL(1,column-1,BLUE_VALUE) + IMAGE_PIXEL(1,column+1,BLUE_VALUE) ) /2;
	for(line=2; line<image->height-1; line+=2)  // first column at R positions
	  image->pixels[ PIXEL_INDEX(line,0,BLUE_VALUE) ] = ( IMAGE_PIXEL(line-1,1,BLUE_VALUE) + IMAGE_PIXEL(line+1,1,BLUE_VALUE) ) /2;

  return 0;
}

int cr2ImageCreate_new(struct cr2file *cr2, struct cr2image *image, unsigned short *corners, unsigned short **pixels) {
  unsigned long rowWidth;
  unsigned short line, column, l, c, color, pixel;

  *pixels=(unsigned short *) calloc( cr2->width * cr2->height * NB_COLORS, sizeof(short) );
  if (!(*pixels)) {
    fprintf(stderr,"malloc image->pixels\n");
		return -1;
  }	
	  //puts("createImage");
	// we really copy image data
  rowWidth = cr2->sensorWidth;
  for(line = cr2->tBorder+cr2->vShift, l=0; line <= cr2->bBorder+cr2->vShift; line++, l++) 
    for(column = cr2->lBorder, c=0; column <= cr2->rBorder; column++, c++) {
      color = FILTER_COLOR(l, c);
      pixel = cr2->unsliced[ line*rowWidth + column ];
      (*pixels)[ PIXEL_INDEX(l,c,color) ] = pixel; // PIXEL_INDEX() uses image->width
    }
  return 0;  
}

/*
 * create RGGB array at the right RGB column
 */
int cr2ImageCreate(struct cr2file *cr2, struct cr2image *image, unsigned short *corners) {

  /*unsigned short line, column, l, c, color, pixel;
  unsigned long rowWidth;*/
	
  if (cr2->jpeg.bits==15) {
    fprintf(stderr, "createImage: must be RGGB data\n");
    return -1;
  }
  if (image->pixels!=0) {
    fprintf(stderr, "createImage: image already created\n");
    return -1;
  }
  
  if (cr2->width==0 || cr2->height==0) // find borders first
    if (cRaw2FindImageBorders(cr2, corners)!=0)
      return -1;
    
  image->width = cr2->width;	
  image->height = cr2->height;	
  if (cr2ImageCreate_new( cr2, image, corners, &(image->pixels) )!=0)
    return -1;
  if ( hasDualPixel(cr2) ) {
    if ( cr2ImageCreate_new( cr2, image, corners, &(image->pixelsB) )!=0)
      return -1;
  }
  return 0;
	
}

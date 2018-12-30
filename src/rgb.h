#ifndef RGB_H
#define RGB_H 1

#include"craw2.h"
#include<math.h>

// inspired by dcraw FC()
#define FILTER_COLOR(l, c) ((((l)&1)<<1) | (c&1))

#define NB_COLORS 4
#define NB_COLORS_YUV 3

#define INTERPOLATION_BILINEAR 1

#define PIXEL_INDEX(l,c,color) ((l)*image->width*NB_COLORS + (c)*NB_COLORS + color)
#define IMAGE_PIXEL(l,c,color) (image->pixels[PIXEL_INDEX(l,c,color)])

#define RED_VALUE    0
#define GREEN1_VALUE 1
#define GREEN2_VALUE 2
#define BLUE_VALUE   3


struct cr2image {
  unsigned short *pixels;
  unsigned short *pixelsB;
/*  unsigned short black[4], white[4];
  unsigned long counts[4];
  double sums[4], stdev[4];*/
  unsigned short width, height;
  int isRGGB;
};

#include"export.h"


int EXPORT cr2ImageCreate(struct cr2file *cr2, struct cr2image *image, unsigned short *corners);
int EXPORT interpolateRGB_bilinear(struct cr2image *image);
int EXPORT cr2ImageNew(struct cr2image *image);
int EXPORT cr2ImageFree(struct cr2image *image);
int EXPORT computeImageStats(struct cr2image *image);
double EXPORT Log2( double n ); 

#endif // RGB_H
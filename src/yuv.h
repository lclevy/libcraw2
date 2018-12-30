#ifndef YUV_H
#define YUV_H 1

#include"craw2.h" // struct cr2file
#include"rgb.h" // struct cr2image

int EXPORT yuvInterpolate( struct cr2file *cr2 );
int EXPORT yuvPack( struct cr2file *cr2, unsigned short *packed, unsigned long *packedLen );
int EXPORT yuv2rgb( struct cr2file *cr2, struct cr2image *img );
int EXPORT yuvUnPack( struct cr2file *cr2, unsigned short *packed, unsigned long *unPackedLen );
int EXPORT yuvMaskCreate(struct cr2file *cr2); 
int EXPORT yuvComputeImageStats(struct cr2file *cr2);

#endif // YUV_H
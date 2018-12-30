#ifndef BITS_H
#define BITS_H

#include<stdio.h>

struct bitsStreamState {
  unsigned long bitsBuffer; // valid bits are 'bufferLength' rightmost ones
  int bufferLength;
  unsigned long dataOffset; // pointer in file
  unsigned long endOfData; // end offset in file
};

unsigned long getBits(struct bitsStreamState *bss, FILE* file, int nBits );
void shiftBits(struct bitsStreamState *bss, FILE* file, int nBits);
void putBits(struct bitsStreamState *bss, unsigned long bits, int nBits, unsigned char* buf);
int bitsLen(unsigned short bits);
void computeBitMask();

#endif //  BITS_H
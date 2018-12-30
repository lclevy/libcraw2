// bits.c

#include"bits.h"

#include<stdio.h>

extern int cr2Verbose;

unsigned long bitMask[33];
unsigned long bitMaskLeft[33]; // not used

void computeBitMask() {
  unsigned long mask=0, mask2=0;
  int i;
  for(i=1; i<=32; i++) {
    mask|=1;
		mask2|=0x80000000;
    bitMask[i] = mask;
		bitMaskLeft[i] = mask2;
    mask<<=1;
		mask2>>=1;
  }
}

// endOfData (max offset in file) must be set up before usage
// nBits is max 15 bits
unsigned long getBits(struct bitsStreamState *bss, FILE* file, int nBits ) {
  unsigned char byteVal;
  if (nBits==0) { // initialization
    bss->bufferLength = 0;
    bss->bitsBuffer = 0;
    return 0;
  }
  while( bss->bufferLength < nBits && bss->dataOffset < bss->endOfData) {
    byteVal = fgetc(file);
    (bss->dataOffset)+=1;  
    (bss->bitsBuffer) <<= 8;
    (bss->bitsBuffer) |=  byteVal;
    (bss->bufferLength) += 8;
    if (byteVal==0xff) { // 0xff marker, next byte must be read
      byteVal = fgetc(file);
      (bss->dataOffset)+=1;  
      if (byteVal==0xd9) // end of scan
        return bss->bitsBuffer;
	  if (byteVal != 0x00) {
        fprintf(stderr, "0xff followed by 0x%02x\n", byteVal); 
        fprintf(stderr,"ftell=%ld\n", ftell(file));
        // else "ff00" means "ff", so we let the "ff" in bitsBuffer and forget the "00" we just read  
	  }
    }
  }
  return bss->bitsBuffer;
}
void shiftBits(struct bitsStreamState *bss, FILE* file, int nBits) {
  if (bss->bufferLength < nBits) {// if not enough bits are present in bitsBuffer
    getBits(bss, file, nBits);
    //fprintf(stderr,"shiftBits: not enough bits in bitsBuffer\n");
  }
  (bss->bufferLength) -= nBits; // remove nBits from bitsBuffer
  bss->bitsBuffer = bss->bitsBuffer & bitMask[bss->bufferLength]; // and only keep the remaining bits (right justified)
}

int maxlen;

// bufLen in short, nBits is <16
void putBits(struct bitsStreamState *bss, unsigned long bits, int nBits, unsigned char* buf) {
  if (nBits>16) {
    fprintf(stderr,"putBits: nBits = %d\n", nBits);
    return;
  }
  if (nBits==-1) { // init
    maxlen=0;
  //printf("putBits init ");
    bss->bitsBuffer = 0; // bits in local buffer not yet written in "buf"
    bss->bufferLength = 0; // local buffer length
    return;
  }
  if (nBits==-2) { // flush: write remaining bits in buffer, left justified. NEVER TESTED!!!
    while(bss->bufferLength>0) {
 //printf("buf1 =%s n=%d dataOffset=%d\n", binaryPrint(bss->bitsBuffer, bss->bufferLength), bss->bufferLength, bss->dataOffset);
      if (bss->bufferLength>=8) {
        buf[bss->dataOffset] =(unsigned char)(bss->bitsBuffer>>(bss->bufferLength-8));
        bss->bitsBuffer = bss->bitsBuffer & bitMask[bss->bufferLength-8];
        bss->bufferLength -= 8;
      }
      else {
        buf[bss->dataOffset] = ((bss->bitsBuffer<<(8-bss->bufferLength)) | bitMask[8-bss->bufferLength]) & 0xff; 
				// missing bits must be put to 1 (T81, F.1.2.3, page 91)
        if (buf[bss->dataOffset]==0xff) // if padding create a 0xff, put a 00 after
          buf[++bss->dataOffset] = 0;        
        bss->bufferLength = 0;
      }
      bss->dataOffset++;
    }
    //printf("bss->bufferLength %d maxlen=%d\n", bss->bufferLength, maxlen ); 
    
    // putBits must not be reused before init !
    return;
  }
  bss->bitsBuffer = (bss->bitsBuffer<<nBits) | (bits & bitMask[nBits]);
  bss->bufferLength += nBits;    
  
  if (bss->bufferLength>maxlen)
    maxlen = bss->bufferLength;
  while(bss->bufferLength >=8 /*&& bss->dataOffset < bss->endOfData*/) {
    if ( (bss->bitsBuffer>>(bss->bufferLength-8))==0xff ) {
      buf[bss->dataOffset++] = 0xff;
      buf[bss->dataOffset++] = 0x00;
    }
    else 
      buf[bss->dataOffset++] = (bss->bitsBuffer>>(bss->bufferLength-8))&0xff;
    
    bss->bufferLength -= 8;
    bss->bitsBuffer = bss->bitsBuffer & bitMask[bss->bufferLength];
  }
  //printf("buf3=%s %d \n ", binaryPrint(bss->bitsBuffer, bss->bufferLength), bss->bufferLength );
  
}

// returns length in bits of value 'bits'
int bitsLen(unsigned short bits) {
  int len=0;
  while(bits>0) {
    len++;
    bits>>=1;
  }
  return len;
}

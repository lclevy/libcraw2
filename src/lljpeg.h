#ifndef LLJPEG_H
#define LLJPEG_H

#include"utils.h"
#include<stdio.h>
#include<stdlib.h>

// http://msdn.microsoft.com/en-us/library/ms235636%28v=vs.90%29.aspx

#include"export.h"
#include<stdint.h>
/*
#ifndef fGet16
#define fGet16(f) ((fgetc(f)<<8) | fgetc(f))
#define fGet32(f) (fGet16(f)<<16) | fGet16(f)

#define fiGet16(f) (fgetc(f) | (fgetc(f)<<8))
#define fiGet32(f) (fiGet16(f)) | (fiGet16(f)<<16)
#endif // fGet16
*/
#define JPEG_SOI        0xffd8
#define JPEG_APP1       0xffe1
#define JPEG_DHT        0xffc4
#define JPEG_DQT        0xffdb
#define JPEG_SOF0       0xffc0
#define JPEG_SOF3       0xffc3
#define JPEG_SOS        0xffda
#define JPEG_EOI        0xffd9

#define DHT_NBCODE 16

struct huffmanCode {
  int codeLen;
  unsigned short huffCode;
  int value;
};


#define MAX_HUFFMAN_TABLES 4
#define MAX_COMP 4
struct lljpeg{
  int dhtSize; // to patch jpeg headers later
  unsigned char *dht;
  int nHuffmanTables; // how many Huffman tables in DHT section
  uint32_t *huffmanTables[MAX_HUFFMAN_TABLES]; // fast lookup table for Huffman code to value and codeLen
  int info[MAX_HUFFMAN_TABLES]; // from JPEG_DHT, bits0-3 : index, bit4 : type (0=DC, 1=AC)
  unsigned char bits, ncomp;  // from JPEG_SOF3
  unsigned short wide, high;
  unsigned short maxCodeLen[MAX_HUFFMAN_TABLES];
  unsigned char hv[MAX_COMP]; // from JPEG_SOF3, horizontal and vertical sampling factors
  unsigned char dCaCTable[MAX_COMP]; // from JPEG_SOS
  unsigned short huffman2Value[MAX_HUFFMAN_TABLES][DHT_NBCODE][3]; // codelen, huffcode, value. ordered by Huffman codes
  int nCodes[MAX_HUFFMAN_TABLES];
  struct huffmanCode value2Huffman[MAX_HUFFMAN_TABLES][DHT_NBCODE]; // codeLen, Huffman code. order by value (0 to 15)
  unsigned long scanDataOffset; // where jpeg compressed data really start
  unsigned short *rawBuffer; // filled by decompress()
  unsigned long rawBufferLen;
	int hsf, vsf;
  int hvComp[MAX_COMP], hvCount;
};

struct cr2file; // forward definition

int EXPORT jpegParseHeaders(FILE *file, unsigned long stripOffsets, struct lljpeg *jpeg) ;
int EXPORT jpegDecompress( struct lljpeg *jpeg, FILE *file, unsigned long stripOffsets, unsigned long stripByteCounts); 
int EXPORT jpegCompress(struct lljpeg *jpeg, unsigned short *in, unsigned char *out, unsigned long *outLen, 
					struct huffmanCode value2Huffman[MAX_HUFFMAN_TABLES][DHT_NBCODE], int nCodes[MAX_HUFFMAN_TABLES] );
void EXPORT recomputeHuffmanCodes( struct lljpeg *jpeg, struct huffmanCode value2Huffman[MAX_HUFFMAN_TABLES][DHT_NBCODE], int nCodes[MAX_HUFFMAN_TABLES]);
int EXPORT jpegCreateDhtSection( int nHuffmanTables, struct huffmanCode value2Huffman[MAX_HUFFMAN_TABLES][DHT_NBCODE], int nCodes[MAX_HUFFMAN_TABLES], unsigned char* dhtSection);
void EXPORT jpegPrint(struct lljpeg *jpeg);
void EXPORT jpegFree(struct lljpeg *jpeg);
void EXPORT jpegNew(struct lljpeg *jpeg);

void EXPORT displayJpegHeader(int jpegh);
void EXPORT displayLineJpeg( int line );

#endif // LLJPEG_H

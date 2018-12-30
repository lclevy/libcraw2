/*
 * lljpeg.c
 *
 * lossless jpeg (ITU-T81) decompression/compression functions 
 */ 

#include"lljpeg.h"
#include"craw2.h"
#include"bits.h"

#include<stdio.h>

int pixelVerbose=-10000;
int jpegHeader=0;
extern int cr2Verbose;

extern unsigned long bitMask[33];

void displayJpegHeader(int jpegh) {
  jpegHeader = jpegh;
}


void displayLineJpeg( int line ) {
  pixelVerbose = line;
}

void jpegNew(struct lljpeg *jpeg) {
  jpeg->nHuffmanTables = 0;
  jpeg->rawBuffer = 0;
  jpeg->rawBufferLen = 0;
  jpeg->dht=0; 
}

/*
 * Parses DHT data, computes Huffman codes
 * and stores it in jpeg->huffman2Value[], which is later used to generate HuffmanLUT
 * See http://w3-o.cs.hm.edu/~ruckert/understanding_mp3/hc.pdf, pages 156 and 157
 */
int analyzeDHT(unsigned char *dht, struct lljpeg *jpeg) {
  unsigned char nbCodePerSizes[DHT_NBCODE], code; 
  int i, j, codeLen, n;
  unsigned short huffCode=0; 
 
  // jpeg->nHuffmanTables is the index of the next table available. Table is allocated later in this function
  jpeg->info[jpeg->nHuffmanTables] = dht[0]; // bits0-3 : index, bit4 : type (0=DC, 1=AC)
  if (cr2Verbose>0) {
    if ( jpeg->info[jpeg->nHuffmanTables] &0x10 )
      printf("AC, ");
    else  
      printf("DC, ");
    printf("Destination= %d\n", jpeg->info[jpeg->nHuffmanTables] &0xf);  
  }
  memcpy(nbCodePerSizes, dht+1, DHT_NBCODE);
  
  /* determine max Huffman code len, to know the length of the lookup table to allocate */
  i=DHT_NBCODE-1;
  while(nbCodePerSizes[i]==0 && i>=0)
    i--;
  // jpeg->huffmanTables[jpeg->nHuffmanTables] is supposed available
  jpeg->maxCodeLen[jpeg->nHuffmanTables] = i+1;  
  if (cr2Verbose>0)
    printf(" maxCodeLen=%d\n",jpeg->maxCodeLen[jpeg->nHuffmanTables]);  

  if (cr2Verbose>0) 
    printf(" nbCodePerSizes: %s\n", hexlify(nbCodePerSizes, DHT_NBCODE) );
  for(i=0; i<DHT_NBCODE; i++)
    jpeg->value2Huffman[jpeg->nHuffmanTables][i].codeLen = 0; // init
    
  n=0;
  for(i=0; i<DHT_NBCODE; i++) {
      
    codeLen = i+1;
    if (cr2Verbose>0) 
      printf("  number of codes of length %2d bits: %d ( ", codeLen, nbCodePerSizes[i]) ;
    for(j=0; j<nbCodePerSizes[i]; j++) {
      code = dht[1+DHT_NBCODE+n]; // 'code' is remplaced in file by his 'Huffman code'. more frequent values are shortest huffman codes
      if (cr2Verbose>0)
        printf("%02x:%s, ", code, binaryPrint(huffCode, codeLen) );
      // See http://www.w3.org/Graphics/JPEG/itu-t81.pdf, section B.2.4.2, and Annex C
      
      // used to generate LUT, ordered by huffman code
      jpeg->huffman2Value[jpeg->nHuffmanTables][n][0] = codeLen;
      jpeg->huffman2Value[jpeg->nHuffmanTables][n][1] = huffCode;
      jpeg->huffman2Value[jpeg->nHuffmanTables][n][2] = code;
      n++;
      // index is value, used for compression 
      jpeg->value2Huffman[jpeg->nHuffmanTables][code].codeLen = codeLen;  
      jpeg->value2Huffman[jpeg->nHuffmanTables][code].huffCode = huffCode;  
      jpeg->value2Huffman[jpeg->nHuffmanTables][code].value = code; // when not used with code as index (in jpegCreateDhtSection)
      //printf("%d %d %d\n", code, jpeg->value2Huffman[jpeg->nHuffmanTables][code].codeLen, jpeg->value2Huffman[jpeg->nHuffmanTables][code].huffCode);
     
      huffCode++; // increment huffcode value
    }
    huffCode<<=1; // increment huffcode length
    
    if (cr2Verbose>0)
      printf(")\n");
    //nbValues = nbValues + nbCodePerSizes[i];
  }
  if (cr2Verbose>0)
    printf(" totalNumberOfCodes= %d\n", n);
  jpeg->nCodes[jpeg->nHuffmanTables] = n;
  
  return n;
}

/*
 * generate Look-Up Table to find the encoded value based on huffman code and length
 * each entry has 28 MSB for value and 4 LSB for length
 */
int generateHuffmanLUT(struct lljpeg *jpeg, int table) {
  unsigned long prevIndex = 0, suffix, prefix, indexRange, k;
  int i, val, codeLen;
  unsigned short huffCode=0; 
  
  jpeg->huffmanTables[table] = (uint32_t *)malloc( (1<<(jpeg->maxCodeLen[table])) * sizeof(uint32_t) );
  if (!jpeg->huffmanTables[table]) {
    printf("malloc error");
    return -1;
  }
  for(i=0; i<jpeg->nCodes[table]; i++) {
    codeLen = jpeg->huffman2Value[jpeg->nHuffmanTables][i][0];
    huffCode = jpeg->huffman2Value[jpeg->nHuffmanTables][i][1];
    val = jpeg->huffman2Value[jpeg->nHuffmanTables][i][2];
//printf(" val=%2d codeLen=%2d huff=%-16s\n", val, codeLen, binaryPrint(huffCode, codeLen) );
    // generate Huffman table: each entry has 28 MSB for value and 4 LSB for length 
    suffix = 0;
    prefix = huffCode<<(jpeg->maxCodeLen[table] - codeLen);
    indexRange = 1<<(jpeg->maxCodeLen[table] - codeLen);
    //printf("prefix=%lx prevIndex=%lx endIndex=%lx val=%x\n", prefix, prevIndex, indexRange+prevIndex, val<<4 | codeLen );
    for(k=prevIndex; k< (prevIndex+indexRange); k++) {
      jpeg->huffmanTables[ table ][ prefix | suffix ] = (val<<4) | codeLen;
      suffix++;
    }
    prevIndex = prevIndex+indexRange;
    // end of table generation
  }
  return 0;
}

//See http://w3-o.cs.hm.edu/~ruckert/understanding_mp3/hc.pdf, pages 156 and 157
//huffVal must be < jpeg->maxCodeLen
void findHuffmanCode(uint32_t *huffTable, unsigned short huffVal, int *codeLen, unsigned short *val, int maxCodeLen) {
  unsigned long entry;
  huffVal &=bitMask[maxCodeLen];
  entry = huffTable[ huffVal ];
  //printf("entry=%x ",entry);
  *codeLen = entry & 0xF; // 4 right most bits
  *val = (unsigned short)(entry>>4); // original encoded value
}

int currentLine;

int getDiffValue(struct bitsStreamState *bss, FILE* file, struct lljpeg *jpeg, int huffTableNum) {
  unsigned short candidateHuffCode, value;
  int diffVal;
  int huffCodeLen;
  getBits(bss, file, jpeg->maxCodeLen[huffTableNum]); // encoded length of next diff value
  if (cr2Verbose>3 && currentLine==(pixelVerbose/jpeg->vsf) )
    printf("bufferLength=%d %s\n", bss->bufferLength, binaryPrint(bss->bitsBuffer, bss->bufferLength ) );
  if (bss->bufferLength < jpeg->maxCodeLen[huffTableNum]) 
    candidateHuffCode = (unsigned short)(bss->bitsBuffer<<(jpeg->maxCodeLen[huffTableNum] - bss->bufferLength)); // left ajusted to jpeg->maxCodeLen[huffTableNum] bits
	else
    candidateHuffCode = (unsigned short)(bss->bitsBuffer>>(bss->bufferLength - jpeg->maxCodeLen[huffTableNum])); // right ajusted 
  findHuffmanCode( jpeg->huffmanTables[ huffTableNum ], candidateHuffCode, &huffCodeLen, &value, jpeg->maxCodeLen[huffTableNum] );
  // "value" is the length of following "diff value" in bits 
  if (cr2Verbose>2 && currentLine==(pixelVerbose/jpeg->vsf) )
    printf("l=%s(%d) ", binaryPrint( bss->bitsBuffer>>(bss->bufferLength - huffCodeLen), huffCodeLen), value);
  shiftBits(bss, file, huffCodeLen);
  
  if (value==0) 
    diffVal = 0;
  else {
    getBits(bss, file, value);
    if (cr2Verbose>3 && currentLine==(pixelVerbose/jpeg->vsf) )
		  printf("getBits=%lx bufferLength=%d\n", bss->bitsBuffer, bss->bufferLength );
    diffVal = (int) bss->bitsBuffer>>(bss->bufferLength-value);
    if (cr2Verbose>2 && currentLine==(pixelVerbose/jpeg->vsf))
      printf("d=%s ", binaryPrint( bss->bitsBuffer>>(bss->bufferLength - value), value) );
    shiftBits(bss, file, value);
  }
  if (cr2Verbose>2 && currentLine==(pixelVerbose/jpeg->vsf))
    printf("diff1=%d, ", diffVal);
  // see Table H.2 in itu-t81.pdf
  if ( ( diffVal & ( 1<<(value-1) ) ) == 0)  // if MSB is 0, diff is positive, else negative
    diffVal = (int)- ( diffVal ^ bitMask[value] );   // dcraw does "diff -= (1 << len) - 1"
  if (cr2Verbose>2 && currentLine==(pixelVerbose/jpeg->vsf))
    printf("diff2=%d ", diffVal);

  return diffVal;
}

/*
 * jpegDecompress(): Decode Lossless Sequential Huffman stream. See itu-t81.pdf, Annex H, page 132
 *
 * jpegParseHeaders() MUST be called first to get Jpeg parameters
 * required inputs from the CR2 file structure: cr2->file, cr2->stripOffsets, cr2->stripByteCounts
 *                  
 * output: fills jpeg->rawBuffer, jpeg->rawBufferLen (size of buffer in "short")
 * rawBuffer is a "short" array, filled with data AS DECOMPRESSED. Data is still "sliced". 
 *   format is 
 *     RGRG.. (odd lines) GBGB.. (even lines) for RAW, 
 *     Y1Y2Y3Y4CbCr... for MRAW, 
 *     Y1Y2CbCr... for SRAW. 
 *       SRAW and MRAW are not "normalized": Cr and Cb are not converted to signed data (sign extension) 
 */
char *yuvStr[3]={"Y", "Cb", "Cr"}; 
 
int jpegDecompress( struct lljpeg *jpeg, FILE *file, unsigned long stripOffsets, unsigned long stripByteCounts ) { // cr2->jpeg, cr2->file, cr2->stripOffsets, cr2->stripByteCounts
  struct bitsStreamState bss;
  int line, column, comp, hv;
  unsigned short pixel;
  int diffVal, inMCU;
  unsigned short prevPixel[MAX_COMP];
  int h, w;
  
  if (jpeg->nHuffmanTables==0) {
    fprintf(stderr, "jpegDecompress: no huffman tables, jpeg headers must be analysed before decompression!\n");
    return -1;
  }
  
  computeBitMask();
  for(comp=0; comp<jpeg->ncomp; comp++ ) 
    prevPixel[comp] = (1<<(jpeg->bits-1));              //default previous pixel value per component

  h = jpeg->high / jpeg->vsf;       
  w = jpeg->wide / jpeg->hsf; 
	
  jpeg->rawBufferLen = h * w * jpeg->hvCount; // size in short
  jpeg->rawBuffer = malloc( sizeof(unsigned short) * jpeg->rawBufferLen );
  if (!jpeg->rawBuffer) {
    fprintf(stderr,"jpegDecompress: image->rawData = malloc error\n");
    return -1;
  }
  fseek(file, jpeg->scanDataOffset, SEEK_SET);

  bss.dataOffset = jpeg->scanDataOffset;
  if (cr2Verbose>2)
    printf("scanDataOffset=0x%lx\n", jpeg->scanDataOffset);
  bss.endOfData = stripOffsets + stripByteCounts;
  getBits(&bss, 0, 0 ); // buffer init
  
  for(line=0; line<h; line++) {
    currentLine = line; // debug
    for(column=0; column<w; column++) {  
      inMCU = 0;
      for(comp=0; comp<jpeg->ncomp; comp++ ) {   // 3 (sraw/mraw), 2 or 4 components (RAW)
        for(hv=0; hv<jpeg->hvComp[comp]; hv++) { // hvComp[0] = 4, 2 (sraw/mraw) or 1 (RAW)

          diffVal = getDiffValue(&bss, file, jpeg, (jpeg->dCaCTable[ comp ] & 0xf0)>>4); // dc value as Huffman table number
        
          // at beginning of lines (column==0) with line>=1, "previous values" of a given component is taken on the previous line
          if (hv==0 && column==0 && line>=1) 
            prevPixel[comp] = jpeg->rawBuffer[w*jpeg->hvCount*(line-1) + inMCU];

          pixel = diffVal + prevPixel[comp]; // current pixel is "previous pixel" + difference value
          jpeg->rawBuffer[w*jpeg->hvCount*line + column*jpeg->hvCount +inMCU] = pixel; 
          
          if (cr2Verbose>2 && line==(pixelVerbose/jpeg->vsf) ) {
            if (jpeg->hsf==1) // RAW, but can not know which of R,G1,G2,B, must unslice() and findBorders() first
              printf("p=0x%x / %d\n ", pixel, pixel);
            else { // jpeg->hsf>1
              if (comp==0) // Y
                printf("p=0x%x / %d (%s)\n ", pixel, pixel, yuvStr[comp]);
              else  // comp>0 : Cb or Cr = signed
                printf("p=0x%x / %d (%s) \n ", pixel, (short)pixel-16384, yuvStr[comp]);
            }
          }  
          prevPixel[comp] = pixel;
          inMCU++;  
        } // for hv
      } // for comp
    } // for column
    //printf("line=%d ftell=%d \n", line, ftell(cr2->file)-jpeg->scanDataOffset);
  } // for line 
  
  return 0;
}
 
/* 
 * compress data in rawBuffer into out[] buffer, using optionally Huffman given
 * if value2Huffman[] is not null, use it, instead use jpeg->value2Huffman[]
 */
int jpegCompress(struct lljpeg *jpeg, unsigned short *in, unsigned char *out, unsigned long *outLen, 
	struct huffmanCode value2Huffman[MAX_HUFFMAN_TABLES][DHT_NBCODE], int nCodes[MAX_HUFFMAN_TABLES]) {

  struct bitsStreamState bss;
  long comp, hv, line, column, n, h, w;
  int huffCodeLen, huffTableNum, diffLen, diffVal, inMCU;
  unsigned short huffCode, dcValue, *rawBuffer;
  unsigned short prevPixel[MAX_COMP];
  unsigned long rawBufferLen;
/*  int i, t;
	for(t=0; t<jpeg->nHuffmanTables; t++)
	  for(i=0; i<nCodes[t]; i++) 
      printf("%2d: codeLen=%2d %-10s val=%d\n", i, value2Huffman[t][i].codeLen,  binaryPrint(value2Huffman[t][i].huffCode, value2Huffman[t][i].codeLen),
          value2Huffman[t][i].value);*/

	if (!in) 
	  rawBuffer = jpeg->rawBuffer;
  else
	  rawBuffer = in;
  rawBufferLen = jpeg->rawBufferLen;
  for(comp=0; comp<jpeg->ncomp; comp++ ) 
    prevPixel[comp] = (1<<(jpeg->bits-1));              //default previous pixel value per component

  bss.dataOffset = 0;
  bss.endOfData = rawBufferLen;
  putBits(&bss, 0, -1, out ); // init
  
  h = jpeg->high / jpeg->vsf;       // vertical sampling factor
  w = jpeg->wide / jpeg->hsf;       // horizontal sampling factor

  n = 0;  
  for(line=0; line<h; line++) {
	  currentLine = line; //debug
    for(column=0; column<w; column++) {
      inMCU = 0;
      for(comp=0; comp<jpeg->ncomp; comp++) {
        huffTableNum = (jpeg->dCaCTable[ comp ] & 0xf0)>>4;
        for(hv=0; hv<jpeg->hvComp[comp]; hv++) {
        
          if (hv==0 && column==0 && line>=1) 
            prevPixel[comp] = rawBuffer[w*jpeg->hvCount*(line-1) + inMCU];

          diffVal = rawBuffer[w*jpeg->hvCount*line + column*jpeg->hvCount +inMCU] - prevPixel[comp]; // current pixel is "previous pixel" + difference value)

          prevPixel[comp] = rawBuffer[w*jpeg->hvCount*line + column*jpeg->hvCount +inMCU];
          inMCU++;  
          
          if (diffVal<0)
            diffLen = bitsLen( -diffVal );
          else 
            diffLen = bitsLen( diffVal );
          
          if (value2Huffman==0) {
          // find Huffman code for difference length  
            huffCodeLen = jpeg->value2Huffman[ huffTableNum ][ diffLen ].codeLen;
            huffCode = jpeg->value2Huffman[ huffTableNum ][ diffLen ].huffCode;
          }
          else {
            huffCodeLen = value2Huffman[ huffTableNum ][ diffLen ].codeLen;
            huffCode = value2Huffman[ huffTableNum ][ diffLen ].huffCode;
          }
          if (cr2Verbose>2 && line==(pixelVerbose/jpeg->vsf) )
					  printf("l=%s(%d) ", binaryPrint(huffCode, huffCodeLen), diffLen);
          putBits(&bss, huffCode, huffCodeLen, out); // write Huffman code for diff length
          // compute DC value
          if (diffVal<0)
            dcValue = ((-diffVal) ^ bitMask[diffLen]) & 0xffff;
          else
            dcValue = diffVal & 0xffff;
            
          if (cr2Verbose>2 && line==(pixelVerbose/jpeg->vsf) )
					  printf("dc=%s diff=%d\n",  binaryPrint(dcValue, diffLen), diffVal );
          if (diffVal!=0)
            putBits(&bss, dcValue, diffLen, out);          
          n++;
        } // for hv
      } // for comp
    } // column
    //printf("line=%d n=%d\n", line, bss.dataOffset);

  } // line
  putBits(&bss, 0, -2, out); // flush bits buffer
  
  //printf("n=%lx\n", bss.dataOffset);
  *outLen = bss.dataOffset;
  return 0;
}

struct huffmanNode{
  struct huffmanNode *left;
  struct huffmanNode *right;
  struct huffmanCode hc;
  unsigned long occurence;
};

int cmpfunc (const void * a, const void * b) // to sort in increasing order by occurence value
{
  unsigned long aa = (*(struct huffmanNode**)a)->occurence;
  unsigned long bb = (*(struct huffmanNode**)b)->occurence;
  return aa-bb;
}

/*
 * generate Huffman tree based on ht->occurence, ht->hc.value 
 */
void generateHT(struct huffmanNode *ht, int d, char s, unsigned short huffCode, int codeLen, struct huffmanCode value2Huffman[DHT_NBCODE]) {
  int i;
  if (ht->left==0 && ht->right==0) { // leaf node
    if (cr2Verbose>1)
      for(i=0; i<d; i++)
        printf("  ");
    ht->hc.codeLen = codeLen;
    ht->hc.huffCode = huffCode; 
    value2Huffman[ht->hc.value].codeLen = ht->hc.codeLen;     
    value2Huffman[ht->hc.value].huffCode = ht->hc.huffCode;     
    value2Huffman[ht->hc.value].value = ht->hc.value;
    if (cr2Verbose>1)
      printf("%c: d=%2d, p=%p, occ=%7ld val=%d hc=%s\n", s, d, ht, ht->occurence, ht->hc.value, binaryPrint(ht->hc.huffCode, ht->hc.codeLen) );
  }
  else {  
    if (ht->left) 
      generateHT(ht->left, d+1, 'l', huffCode<<1, codeLen+1, value2Huffman); // left branch
    if (ht->right) 
      generateHT(ht->right, d+1, 'r', (huffCode<<1) |1, codeLen+1, value2Huffman); // right branch
  }
}

// free memory for Huffman Tree
struct huffmanNode * freeHT(struct huffmanNode *ht) {

  if (ht->left)
    ht->left  = freeHT(ht->left);
  if (ht->right)
    ht->right = freeHT(ht->right);

  if (ht->left==0 && ht->right==0) {
    free(ht);
    return 0;
  }  
  else 
    return ht;
}

/*
 * recompute Huffman codes based in image content
 * seems OK
 */
void recomputeHuffmanCodes( struct lljpeg *jpeg, struct huffmanCode value2Huffman[MAX_HUFFMAN_TABLES][DHT_NBCODE], int nCodes[MAX_HUFFMAN_TABLES]) {
  long comp, hv, line, column,  h, w;
  int huffTableNum, inMCU, diffVal, diffLen, i, t, n;
  unsigned short prevPixel[MAX_COMP];
  struct huffmanNode * lenValueOccurences[MAX_HUFFMAN_TABLES][DHT_NBCODE];
  int maxDiffLen[MAX_HUFFMAN_TABLES];
  struct huffmanNode *hnode;
  unsigned long diffcount;
	
  for(comp=0; comp<jpeg->ncomp; comp++ ) 
    prevPixel[comp] = (1<<(jpeg->bits-1));              //default previous pixel value per component

  for(t=0; t<jpeg->nHuffmanTables; t++ ) {
    for(i=0; i<DHT_NBCODE; i++) {
      lenValueOccurences[t][i]=(struct huffmanNode*)malloc(sizeof(struct huffmanNode)); 
      if (!lenValueOccurences[t][i]) {
        fprintf(stderr,"recomputeHuffmanCodes malloc\n");
        return;
      }
      lenValueOccurences[t][i]->occurence = 0;
      lenValueOccurences[t][i]->left = 0;
      lenValueOccurences[t][i]->right = 0;
      lenValueOccurences[t][i]->hc.codeLen = 0;
      value2Huffman[t][i].codeLen = 0;
    }
  }  

  h = jpeg->high / jpeg->vsf;       
  w = jpeg->wide / jpeg->hsf; 

  for(line=0; line<h; line++) {
    for(column=0; column<w; column++) {
      inMCU = 0;
      for(comp=0; comp<jpeg->ncomp; comp++) {
        huffTableNum = (jpeg->dCaCTable[ comp ] & 0xf0)>>4;
        for(hv=0; hv<jpeg->hvComp[comp]; hv++) {
          // compute difference values from pixel data
          if (hv==0 && column==0 && line>=1) 
            prevPixel[comp] = jpeg->rawBuffer[w*jpeg->hvCount*(line-1) + inMCU];

          diffVal = jpeg->rawBuffer[w*jpeg->hvCount*line + column*jpeg->hvCount +inMCU] - prevPixel[comp]; // current pixel is "previous pixel" + difference value)

          prevPixel[comp] = jpeg->rawBuffer[w*jpeg->hvCount*line + column*jpeg->hvCount +inMCU];
          inMCU++;  
          
          if (diffVal<0)
            diffLen = bitsLen( -diffVal );
          else 
            diffLen = bitsLen( diffVal );
          // stores occurences of diffLen values for huffman coding
          lenValueOccurences[huffTableNum][diffLen]->occurence++;
          lenValueOccurences[huffTableNum][diffLen]->hc.value = diffLen;
        } // for hv
      } // for comp
    } // column
    //printf("line=%d n=%d\n", line, bss.dataOffset);
  } // line

  // sort values by increasing number of occurrences and remove values with no occurrences 
  for(t=0; t<jpeg->nHuffmanTables; t++ ) {
    maxDiffLen[t] = DHT_NBCODE; // how many different values
    qsort(lenValueOccurences[t], DHT_NBCODE, sizeof(struct huffmanNode*), cmpfunc );
    for(i=0; i<DHT_NBCODE; i++) 
      if (lenValueOccurences[t][i]->occurence == 0) {
        free(lenValueOccurences[t][i]);
        lenValueOccurences[t][i] = 0;
        maxDiffLen[t]--;
      }
  }
  // shift the table up to remove deleted entries
  for(t=0; t<jpeg->nHuffmanTables; t++ ) 
    for(i=0; i<maxDiffLen[t]; i++) {
      lenValueOccurences[t][i] = lenValueOccurences[t][i+(DHT_NBCODE-maxDiffLen[t])];
      lenValueOccurences[t][i+(DHT_NBCODE-maxDiffLen[t])] = 0;
    }

  // build Huffman tree
  for(t=0; t<jpeg->nHuffmanTables; t++ ) {
	  diffcount=0;
    for(i=0; i<DHT_NBCODE; i++) 
      if (lenValueOccurences[t][i]!=0)
        diffcount+=lenValueOccurences[t][i]->occurence;

		if (cr2Verbose>1)
      for(i=0; i<DHT_NBCODE; i++) 
        if (lenValueOccurences[t][i]!=0)
          printf("%2d: occ=%7ld len=%2d p=%p %2.5f%%\n", i, lenValueOccurences[t][i]->occurence, lenValueOccurences[t][i]->hc.value, lenValueOccurences[t][i],
					(lenValueOccurences[t][i]->occurence*100.0F)/diffcount);

    n = maxDiffLen[t];
    while(n>1) {
      hnode=(struct huffmanNode*)malloc(sizeof(struct huffmanNode)); // for the sum of two smallest nodes
      if (!hnode) {
        fprintf(stderr,"recomputeHuffmanCodes malloc hnode\n");
        return;
      }  
      hnode->left = lenValueOccurences[t][0]; // smallest on left branch
      hnode->right = lenValueOccurences[t][1];
      hnode->occurence = lenValueOccurences[t][0]->occurence + lenValueOccurences[t][1]->occurence; 
      hnode->hc.codeLen = 0; // no huffcode associated
			hnode->hc.value = -1; // composite node
      if (cr2Verbose>2) {
			  for(i=0; i<2; i++) 
          printf("%2d: occ=%7ld len=%2d p=%p %2.5f%%\n", i, lenValueOccurences[t][i]->occurence, lenValueOccurences[t][i]->hc.value, lenValueOccurences[t][i],
					(lenValueOccurences[t][i]->occurence*100.0F)/diffcount);
        printf("occ=%7ld l=%p(%7ld) r=%p(%7ld) p=%p n=%d\n", hnode->occurence, hnode->left, lenValueOccurences[t][0]->occurence, hnode->right, 
        lenValueOccurences[t][1]->occurence, hnode, n);
      }
			lenValueOccurences[t][0] = hnode;
      n--;
      for(i=1; i<n; i++) {
        lenValueOccurences[t][i] = lenValueOccurences[t][i+1];
      }
      qsort(lenValueOccurences[t], n, sizeof(struct huffmanNode*), cmpfunc );
    }
    generateHT(lenValueOccurences[t][0], 0, ' ', 0, 0, value2Huffman[t]);
  
    if (cr2Verbose>1) 
      printf("new Huffman table#%d (based on image data)\n", t+1);
		nCodes[t]=0;	
    for(i=0; i<DHT_NBCODE; i++) 
      if (value2Huffman[t][i].codeLen!=0) {
			  nCodes[t]++;
        if (cr2Verbose>1) 
          printf("%2d: codeLen=%2d %-10s val=%d\n", i, value2Huffman[t][i].codeLen,  binaryPrint(value2Huffman[t][i].huffCode, value2Huffman[t][i].codeLen),
          value2Huffman[t][i].value);
      }
    freeHT(lenValueOccurences[t][0]);
  }

  return;
}

int cmpHC (const void * a, const void * b) // to sort based on Huffman code
{
  unsigned long aa = (*(struct huffmanCode**)a)->huffCode;
  unsigned long bb = (*(struct huffmanCode**)b)->huffCode;
  return aa-bb;
}

/* 
 * create a jpeg DHT section from a struct huffmanCode table
 * problem: recreated huffman tree seem OK, but can not recreate a DHT
 */
int jpegCreateDhtSection( int nHuffmanTables, struct huffmanCode value2Huffman[MAX_HUFFMAN_TABLES][DHT_NBCODE], int nCodes[MAX_HUFFMAN_TABLES], unsigned char* dhtSection)
{
  int i=0, j, t, n[DHT_NBCODE], x;
  struct huffmanCode *table[DHT_NBCODE];
	
  dhtSection[i++] = JPEG_DHT>>8;
  dhtSection[i++] = JPEG_DHT&0xff;
  dhtSection[i++] = 0; 
  dhtSection[i++] = 2; // length <256
  for(t=0; t<nHuffmanTables; t++) {
    dhtSection[i++] = 0 | (t&0xf); // DC table | index
    for(j=0; j<DHT_NBCODE; j++)
    n[j]=0;
    x=0;	//how many codes
    for(j=0; j<DHT_NBCODE; j++)
      if (value2Huffman[t][j].codeLen!=0) {
        if (cr2Verbose>2)
          printf("%2d: codeLen=%2d %-10s val=%d\n", j, value2Huffman[t][j].codeLen,  binaryPrint(value2Huffman[t][j].huffCode, value2Huffman[t][j].codeLen), value2Huffman[t][j].value);

        n[ value2Huffman[t][j].codeLen -1 ]++; // how many codes of "codeLen" at index "codeLen-1"
        table[x] = (struct huffmanCode *)malloc(sizeof(struct huffmanCode));
        if (!table[x]) {
          fprintf(stderr, "jpegCreateDhtSection: malloc error\n");
          return -1;
        }
        table[x]->codeLen = value2Huffman[t][j].codeLen;
        table[x]->huffCode = value2Huffman[t][j].huffCode;
        table[x]->value = value2Huffman[t][j].value;
        x++;
      }
    for(j=0; j<DHT_NBCODE; j++)
      dhtSection[i++] = n[j]; // fill section with "how many codes per len"

    if (x!=	nCodes[t])
      printf("x!=	nCodes[t], %d!=%d\n", x, nCodes[t]);
    qsort( table, x, sizeof(struct huffmanCode*), cmpHC ); // sort "struct huffmanCode*" per increasing codeLen

    if (cr2Verbose>2)
      for(j=0; j<x; j++)
        if (table[j]->codeLen!=0) 
  	      printf("%2d: codeLen=%2d %-10s val=%d\n", j, table[j]->codeLen,  binaryPrint(table[j]->huffCode, table[j]->codeLen), table[j]->value);

    for(j=0; j<x; j++) 
      dhtSection[i++] = table[j]->value; // store value by increasing codeLen
        
    for(j=0; j<x; j++) {
      /*if (cr2Verbose>0)
			  printf("%2d: codeLen=%2d %-10s val=%d\n", j, table[j]->codeLen,  binaryPrint(table[j]->huffCode, table[j]->codeLen), table[j]->value);*/
      free(table[j]);
    }
    dhtSection[3] += 1+16+x;	
  }	// for(t=0; t<nHuffmanTables; t++)
  printf("table len=%d\n", i);
  return 0; // OK
}

int getHVFactor(unsigned char hv) {
  return ((hv&0xF0)>>4) * (hv&0xF);
}

uint16_t fGet16(FILE *f) {
	uint8_t a = fgetc(f);
	uint8_t b = fgetc(f);
	return (a<<8) | b;
}

/*
 * parses DHT, SOF3 and SOS sections of the lossless JPEG
 */
int jpegParseHeaders(FILE *file, unsigned long stripOffsets, struct lljpeg *jpeg) {
  unsigned long val, size, i, ns;
  int hv, qt, index, cs, dcac, comp, nbValues;
  char sosRead=0;
  jpeg->nHuffmanTables = 0;

  if (!file) {
    fprintf(stderr, "jpegParseHeaders: file is NULL\n");
    return -1;
  } 
  if (!stripOffsets) {
    fprintf(stderr, "jpegParseHeaders: stripOffsets is 0\n");
    return -1;
  } 
  
  fseek(file, stripOffsets, SEEK_SET); // jump to start of jpeg data
  val = fGet16(file);
  if (val!=JPEG_SOI) 
    return (-1); // ERROR
  
  val = fGet16(file);
  do {
	switch(val) {
    case JPEG_DHT:
      val = fGet16(file);
      jpeg->dhtSize = val; // also in dht[1]
      jpeg->dht = malloc(jpeg->dhtSize -2);
      if (!jpeg->dht) {
        fprintf(stderr,"malloc jpeg->dht\n");
        return -1;
      }
      if (cr2Verbose>0 || jpegHeader)
        printf("\nJPEG_DHT, length=0x%lx\n", val);
      val -= 2; // remove length space  
      fread(jpeg->dht, sizeof(char), jpeg->dhtSize - 2, file); //dht can come from original file or recomputed
      i=0;

      while(i<val) {
        nbValues = analyzeDHT(jpeg->dht+i, jpeg);
        generateHuffmanLUT(jpeg, jpeg->nHuffmanTables);
        i += (1 + 16 + nbValues);
        jpeg->nHuffmanTables = jpeg->nHuffmanTables +1;
      }
      free(jpeg->dht);
      jpeg->dht=0;
      break;
    case JPEG_SOF3:
      size = fGet16(file);
      if (cr2Verbose>0 || jpegHeader)
        printf("JPEG_SOF%ld, length=0x%lx\n", val&0xf, size);
      jpeg->bits = fgetc(file);
      jpeg->high = fGet16(file);
      jpeg->wide = fGet16(file);
      jpeg->ncomp = fgetc(file);
      if (jpeg->ncomp > MAX_COMP) {
        fprintf(stderr,"ncomp>MAX_COMP\n" );
        return -1;
      }
      if (cr2Verbose>0 || jpegHeader)
        printf(" bits=%d, high=%d, wide=%d, nb comp=%d\n", jpeg->bits, jpeg->high, jpeg->wide, jpeg->ncomp);
        
      jpeg->hvCount = 0;
      for(comp=0; comp<jpeg->ncomp; comp++) { 
        index = fgetc(file);
        hv = fgetc(file);
        qt = fgetc(file);
        if (cr2Verbose>0 || jpegHeader)
          printf("  index=%d, h=%d, v=%d, qt=%d\n", index, hv>>4, hv & 0xf, qt);
        jpeg->hv[comp] = hv;
				if (comp==0) {
					jpeg->hsf = (jpeg->hv[comp]&0xF0)>>4; // horizontal sampling factor
          jpeg->vsf = jpeg->hv[comp]&0xF; // vertical sampling factor
        }  
        jpeg->hvComp[comp] = getHVFactor( jpeg->hv[comp] ); // total values per component : 4,1,1 for sraw1/mraw (Y1Y2Y3Y4CbCr),  2,1,1 for sraw2/sraw (Y1Y2CbCr)
        jpeg->hvCount += jpeg->hvComp[comp];              // total component values per MCU 
      }			
      break;
    case JPEG_SOS:
      size = fGet16(file);
      if (cr2Verbose>0 || jpegHeader)
        printf("JPEG_SOS, length=0x%lx\n", size);
      ns = fgetc(file);
      if (cr2Verbose>0 || jpegHeader)
        printf(" ns=%ld\n", ns );
      for(i=0; i<ns; i++) {
        cs = fgetc(file);
        dcac = fgetc(file);
        if (cr2Verbose>0 || jpegHeader)
          printf("  cs%d dc%d ac%d\n",  cs, dcac>>4, dcac&0xf );
        jpeg->dCaCTable[i] = dcac;
      }
      if (cr2Verbose>0 || jpegHeader) {      
        printf(" ss=%d", fgetc(file) );
        printf(" se=%d", fgetc(file) );
        printf(" ah/al=0x%01x\n", fgetc(file) );
      }
      else
        fseek(file, 3, SEEK_CUR);
      sosRead = 1;
      jpeg->scanDataOffset = ftell(file);
      break;
    default:
      if (cr2Verbose>0 || jpegHeader)
        printf("??? %lx\n", val);
    }
    val = fGet16(file);
  }while(!sosRead);  

  return 1;
}

// free memory for lljpeg struct
void jpegFree(struct lljpeg *jpeg) {
  int i;
  for(i=0; i<jpeg->nHuffmanTables; i++) {
    free(jpeg->huffmanTables[i]); 
  }
  if (jpeg->rawBuffer)
    free(jpeg->rawBuffer); 
	if (jpeg->dht)
	  free(jpeg->dht);	
}

void jpegPrint(struct lljpeg *jpeg) {
  int i, j;

  if (!jpeg) 
    return;
    
  printf("Bits=%d, High=%d, Wide=%d, nComp=%d\n", jpeg->bits, jpeg->high, jpeg->wide, jpeg->ncomp);
  for(i=0; i< jpeg->ncomp; i++) {
    printf("  HV=%02x DcAc=%02x\n", jpeg->hv[i], jpeg->dCaCTable[i] );
  }
  
  for(i=0; i<jpeg->nHuffmanTables; i++) {
    printf("Huffman table#%d\n", i+1);
    for (j=0; j<jpeg->nCodes[i]; j++) 
      printf(" codelen=%2d huffman=%-15s value=%d\n", jpeg->huffman2Value[i][j][0], binaryPrint(jpeg->huffman2Value[i][j][1], jpeg->huffman2Value[i][j][0]), jpeg->huffman2Value[i][j][2] );
  }
  for(i=0; i<jpeg->nHuffmanTables; i++) {
    printf("Huffman table#%d\n", i+1);
    for(j=0; j<DHT_NBCODE; j++)
      if (jpeg->value2Huffman[i][j].codeLen > 0)
        printf("value=%2d len=%2d code=%s\n", j, jpeg->value2Huffman[i][j].codeLen, binaryPrint(jpeg->value2Huffman[i][j].huffCode, jpeg->value2Huffman[i][j].codeLen) );
  }    
}

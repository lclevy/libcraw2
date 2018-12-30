/* example of libcraw2 library usage */

#include"craw2.h"
#include"lljpeg.h"

#include<stdio.h>
#include<stdlib.h>

int cr2Verbose;

#define OUTFILE_LEN 100

int main(int argc, char* argv[])
{
  struct cr2file cr2Data;
  struct lljpeg jpeg2;
  int i, t, totalCodes, dhtLen, nbValues;
  char *filename=0;
	char outFile[OUTFILE_LEN+1]="";
  FILE* out, *out2;
  unsigned char *scandata, dhtSection[0x60]; // 2 + (1+16+nb_val)*nb_tables
  unsigned long compressedLen;
  struct huffmanCode value2Huffman[MAX_HUFFMAN_TABLES][DHT_NBCODE];
  int nCodes[MAX_HUFFMAN_TABLES];

  // parses command line args
  i=0;
  while (i<argc) {
   if (argv[i][0]=='-') {
     switch(argv[i][1]) {
     case 'v': // verbose level
       i++;
       cr2Verbose = atoi(argv[i]);  
       break;
     case 'o': // output
       i++;
       strncpy(outFile, argv[i], OUTFILE_LEN);  
       break;
     default:
       printf("unknown option %s\n", argv[i]);
     } // switch
   }
   else {
     if (i==(argc-1))
       filename = argv[i];
   }
   i++;   
  } // switch
  puts( filename);
  if (filename) 
    cRaw2Open(&cr2Data, filename );
  else
    exit(1);
    
  cRaw2GetTags( &cr2Data);

  
  jpegParseHeaders(cr2Data.file, cr2Data.stripOffsets, &(cr2Data.jpeg) );  
  //jpegPrint( &jpeg );
  
  jpegDecompress( &(cr2Data.jpeg), cr2Data.file, cr2Data.stripOffsets, cr2Data.stripByteCounts ) ; // decompress into cr2Data.jpeg.rawBuffer
  printf("compression rate=%2.2f%%\n", ((double)(cr2Data.stripByteCounts-(cr2Data.jpeg.scanDataOffset-cr2Data.stripOffsets))*100.0)/((double)cr2Data.jpeg.rawBufferLen*2));
  
  scandata=malloc(sizeof(short) * cr2Data.jpeg.rawBufferLen);
  if (!scandata) {
    fprintf(stderr, "scandata malloc error");
    exit(1);
  }
  printf("rawBufferLen =%ld\n", sizeof(short) *cr2Data.jpeg.rawBufferLen); // in short

	if (1) { // with new huffman codes
    recomputeHuffmanCodes(&(cr2Data.jpeg), value2Huffman, nCodes); // create optimal huffman tree and stores it in value2Human[] and nCodes[]
		jpegNew(&jpeg2); // another jpeg to avoid overwriting existing cr2Data.jpeg metadata 
		
		totalCodes=0; // create a new DHT section, because these will be huffcodes used for compression, not the ones from the tree
		// just compute the DHT size
		for(t=0; t<cr2Data.jpeg.nHuffmanTables; t++) // we suppose the new jpeg will have the same number of Huffman tables
		  totalCodes += nCodes[t]; // total number of values for all tables. may change if image data changed
		dhtLen = 2+2+ cr2Data.jpeg.nHuffmanTables*(1+DHT_NBCODE)	+totalCodes;	//marker + len + ...
		printf("dht len=%d\n", dhtLen);
		jpeg2.dht = malloc(dhtLen);
		if (jpeg2.dht==0) {
		  fprintf(stderr,"malloc jpeg2.dht\n");
		  return -1;
    }
		
    jpegCreateDhtSection( cr2Data.jpeg.nHuffmanTables, value2Huffman, nCodes, jpeg2.dht); 
    // compressed with semi-adaptive Huffman tables, based on image data
		i = 4;
		while(i < jpeg2.dht[3]) { // DHT section size
		printf("i=%d\n",i);
      nbValues = analyzeDHT(jpeg2.dht+i, &jpeg2); // fill jpeg2.value2Huffman[] and jpeg2.nCodes[] with huffman codes from jpeg2 DHT section
      generateHuffmanLUT(&jpeg2, jpeg2.nHuffmanTables);
      i += (1 + 16 + nbValues);
      jpeg2.nHuffmanTables = jpeg2.nHuffmanTables +1;
    }
		
    jpegCompress(&(cr2Data.jpeg), 0, scandata, &compressedLen, jpeg2.value2Huffman, jpeg2.nCodes); // cr2Data.jpeg for image data, jpeg2 for huffman data
	  cRaw2PatchAs("out4.cr2", &cr2Data, scandata, compressedLen, jpeg2.dht);
    jpegFree( &jpeg2 ); 
   	}
	else { // patch with original values
    jpegCompress(&(cr2Data.jpeg), 0, scandata, &compressedLen, 0, 0);
    jpegCreateDhtSection( cr2Data.jpeg.nHuffmanTables, cr2Data.jpeg.value2Huffman, cr2Data.jpeg.nCodes, dhtSection);
	  cRaw2PatchAs("out4.cr2", &cr2Data, scandata, compressedLen, dhtSection);
	}
  printf("compressedLen=%ld\n", compressedLen);
  printf("compression rate=%2.2f%%\n", ((double)compressedLen*100.0)/((double)cr2Data.jpeg.rawBufferLen*2));
 
  out=fopen("craw2_sliced.bin","wb");
  if (out) {
    fwrite(cr2Data.jpeg.rawBuffer, sizeof(unsigned short), cr2Data.jpeg.rawBufferLen, out);
    fclose(out);
  }
  
  free(scandata);

  cRaw2Close(&cr2Data);  

  return(0);
}

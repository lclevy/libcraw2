
#include"utils.h"

#include<string.h>
#include<stdio.h>

#define MAX_BIN_PRINT 200

// prints 'code' in binary, of length 'len', with leading 0 if needed
 char* binaryPrint(unsigned long code, int len) {
  int i;
  static char out[MAX_BIN_PRINT+1];
  if (len>MAX_BIN_PRINT) {
	sprintf((char*)out, "binaryPrint buffer overflow, length=%d!", len);
	return out;
  }
  out[len]='\0';
  for(i=0; i<len; i++) {
    out[len-(i+1)] = (code&1) + '0';
    code >>=1;
  }
  return out;
}

// like the python function
#define HEXLIFY_OUT_LEN 512
#define hexlify_d2a(d) ( ( (d)>=0 && (d)<10 ) ? ('0'+(d)) : ('a'+(d)-10) )
unsigned char* hexlify(unsigned char* data, int dataLen) {
  int i;
  static unsigned char out2[HEXLIFY_OUT_LEN+1];
  if (dataLen> (HEXLIFY_OUT_LEN/2) )
    dataLen=HEXLIFY_OUT_LEN/2;
  for(i=0; i<dataLen; i++) {
    out2[i*2] = hexlify_d2a( (data[i]&0xf0)>>4 );
    out2[(i*2)+1] = hexlify_d2a( data[i]&0xf );
    //printf("%02x %d %c%c\n",  data[i], i*2, out2[i*2], out2[(i*2)+1] );
  }
  out2[dataLen*2] = '\0'; 
  return out2;
}


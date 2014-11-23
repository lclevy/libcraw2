#!/bin/bash

#to generate dng_info.txt, based on DNG tags converted from CR2 images

DNGCONV=/c/Program\ Files\ \(x86\)/Adobe/Adobe\ DNG\ Converter.exe
DNGPIC=converted.dng

for pic in `cat samples_list_rggb.txt`; do
  #convert the CR2 into DNG
  "$DNGCONV" -c -d . -o $DNGPIC $pic
  BLACK=""
  WHITE=""
  MATRIX=""
  if [ -f $DNGPIC ]; #if conversion is OK
  then #extract values with exiftool
    BLACK=`exiftool.exe -s3 -BlackLevel $DNGPIC`
    WHITE=`exiftool.exe -s3 -Whitelevel $DNGPIC`
    MATRIX=`exiftool.exe -s3 -colormatrix2 $DNGPIC`
    rm converted.dng
  fi
  MODELNAME=`src/craw2tool.exe -g 0,272,a $pic`
  MODELID=`src/craw2tool.exe -g m,16,v $pic`
  MODELIDHEX=$(tohex $MODELID)
  echo "$MODELIDHEX"r", $MODELNAME, $BLACK, $WHITE, $MATRIX"
done
for pic in `cat samples_list_yuv.txt`; do
  #convert the CR2 into DNG
  "$DNGCONV" -c -d . -o $DNGPIC $pic
  BLACK=""
  WHITE=""
  MATRIX=""
  if [ -f $DNGPIC ]; #if conversion is OK
  then #extract values with exiftool
    BLACK=`exiftool.exe -s3 -BlackLevel $DNGPIC`
    WHITE=`exiftool.exe -s3 -Whitelevel $DNGPIC`
    MATRIX=`exiftool.exe -s3 -colormatrix2 $DNGPIC`
    rm converted.dng
  fi
  MODELNAME=`src/craw2tool.exe -g 0,272,a $pic`
  MODELID=`src/craw2tool.exe -g m,16,v $pic`
  MODELIDHEX=$(tohex $MODELID)
  echo "$MODELIDHEX"y", $MODELNAME, $BLACK, $WHITE, $MATRIX"
done

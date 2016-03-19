#!/bin/bash 

echo -n "modelId, modelName, jpeg_bits, jpeg_wide, jpeg_high, jpeg_comp, jpeg_hsf, jpeg_vsf, slices[0], slices[1], slices[2], sensorWidth, sensorHeight, sensorLeftBorder, sensorTopBorder, sensorBottomBorder, sensorRightBorder,"
echo "blackMaskLeftBorder,blackMasktopBorder,blackMaskBottomBorder,blackMaskRightBorder, imageWidth, imageHeight, vShift"
for pic in `cat samples_list_rggb.txt samples_list_yuv.txt`; do
  src/craw2tool -m $pic
done
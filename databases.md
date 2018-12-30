# How to generate cr2\_database.txt and dng\_info.txt for new cameras samples ? #

### cr2_database.txt

This CSV table lists Jpeg and Image properties extracted from each CR2 files.
What is required is one CR2 sample for **each** camera and **each** data type: raw, sraw and mraw.

[Imaging Resource](http://www.imaging-resource.com/cameras/reviews/ "Imaging Resource") is a great site to get CR2 samples, but do not provide YUV (mraw and sraw) files.

Then *samples\_list_rggb.txt* and *samples\_list_yuv.txt* must be updated to handles new files, then the *cr2\_database.sh* script can be used.

*craw2tool -m* is used to extract model Jpeg and images properties.

    $ src/craw2tool.exe -m  pics/80DhSLI00100NR2D.CR2
    0x80000350, Canon EOS 80D, 14, 3144, 4056, 2, 1, 1, 1, 3144, 3144, 6288, 4056, 276, 46, 6275, 4045, 0, 0, 0, 0, 6000, 4000, 0

    $ src/craw2tool.exe -m  pics/IMG_0590_mraw.CR2
    0x80000287, Canon EOS 60D, 15, 3888, 2592, 3, 2, 2, 8, 1296, 1296, 3888, 2592, 0, 0, 3887, 2591, 0, 0, 0, 0, 3888, 2592, 0
    
    $ src/craw2tool.exe -m  pics/IMG_0596_sraw.CR2
    0x80000287, Canon EOS 60D, 15, 2592, 1728, 3, 2, 1, 5, 864, 864, 2592, 1728, 0, 0, 2591, 1727, 0, 0, 0, 0, 2592, 1728, 0

**Format is** :
modelId, modelName, 
jpeg\_bits, jpeg\_wide, jpeg\_high, jpeg\_comp, 
slices[0], slices[1], slices[2], jpeg\_hsf, jpeg\_vsf, 
sensorLeftBorder, sensorTopBorder, sensorBottomBorder, SensorRightBorder, blackMaskLeftBorder, blackMasktopBorder,blackMaskBottomBorder,blackMaskRightBorder, imageWidth\*, imageHeight\*, vShift\*

Notes that images dimensions and RGGB borders are **automatically computed** (noted \*) !

vShift==1 means that the first line of sensor data is G2 B,G2 B, ... and not R G1, R G1, ... which seems a bug in Digic 4.

**to generate the database:**

 ``./cr2_database.sh >cr2_database.txt``

some warnings are displayed, no problem.

### dng_info.txt

This database mainly contains color calibration data, like the adobe_coeff() table in dcraw.

The trick is to use [DNG Converter](https://www.adobe.com/support/downloads/product.jsp?product=106&platform=Windows  "DNG Converter") from Adobe to convert CR2 files and the extract these information from the generated dng file.

BlackLevel (0xc61a), WhiteLevel (0xc61d) and ColorMatrix2 (0xc622) DNG tags are extracted using *ExifTool* and camera model and names are extracted using *craw2tool -g* from the CR2 file.

This information is then used for RGB calibration by *pycraw2\_demo.py*

The goal here is to highlight the fact that:

- this information can be extracted **automatically**
- can be used **outside** of libcraw2 code

The only requirement, despite having CR2 samples, is to install the latest DNG converter tool which must support these camera models.

 ``./dng_info.sh >dng_info.txt``

some warnings are displayed, no problem.

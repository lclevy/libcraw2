# libcraw2 API #

v0.7

By "craw2" in the following document, we are talking about data stored in .CR2 files, aka, Canon Raw v2 file format.

## cr2file object ##

    #include"craw2.h"
    struct cr2file cr2;

The cr2file type is used to handle craw2 data.

    if (cRaw2Open(&cr2, filename )<0) 
      exit(1);

### cRaw2Open() (craw2.h) ###
allocates memory and open the file, the file descriptor is stored in *cr2->file*. Returns -1 in case of error, 0 else.

    if ( cRaw2GetTags( &cr2)<0 )
      exit(1);

### cRaw2GetTags() (craw2.h) ###

- checks the file header, 
- parses all TIFF tags and stored them as a linked list in *cr2->tagList*.

Important tags values are also stored for further data processing:

- from IFD#3:
   - stripOffset 
   - stripByteCounts 
   - slices values 
- Camera Model from IFD#0
- from MakerNote:
   - White balance values
   - model Id
   - firmware version
   - sensor info
  - from Exif
   - image width
   - image height

Returns -1 in case of error, 0 else. 

TIFF tags can be displayed using *displayTagList()* or *displayTagListCsv()* from tiff.c

### parsing Jpeg header (jpeg.h) ###

    if (jpegParseHeaders( cr2.file, cr2.stripOffsets, &(cr2.jpeg) )<0)
      exit(1);  

Parses Jpeg headers and prepare structures for decompression and unslicing. Jpeg properties are stored in cr2.jpeg structure.

The following Jpeg properties (see [CR2 jpeg lossless poster](https://github.com/lclevy/libcraw2/blob/master/docs/cr2_lossless.pdf "CR2 jpeg lossless poster")) are stored:

- jpeg.bits
- jpeg.high
- jpeg.wide
- jpeg.ncomp
- jpeg.hsf
- jpeg.vsf

they are displayed with *craw2tool -m*

### decompressing lossless Jpeg data (jpeg.h) ###

    jpegDecompress( &(cr2.jpeg), cr2.file, cr2.stripOffsets, cr2.stripByteCounts ) ;

Data is decompressed into *cr2.jpeg->rawBuffer*, which size is *cr2.jpeg->rawBufferLen*.

### unslicing (craw2.h) ###

    cRaw2Unslice( &cr2 );

Data from *cr2.jpeg->rawBuffer* is unsliced into *cr2->unsliced* which size is *cr2->unslicedLen*.

### for RAW data (RGGB): ###

#### computing image borders ####

    int rc;
    if (corners[2]!=0) // -b option has been used
      rc = cRaw2FindImageBorders(&cr2, corners);
    else	
      rc = cRaw2FindImageBorders(&cr2, 0); // find automatically borders

#### create image ####

    #include"rgb.h"
    struct cr2image image;
    
    cr2ImageNew(&image);
    if (corners[2]!=0) // -b option has been used
      cr2ImageCreate(&cr2, &image, corners);
    else  
      cr2ImageCreate(&cr2, &image, 0);

#### exporting RGGB data as TIFF ####

    #include"tiff.h"
    exportRggbTiff(&cr2, "rggb.tiff" ); // export 16bits RAW greyscale, like dcraw -T -4 -D

image layout is:

     R G1  R G1 ...
    G2  B G2  B ...
     R G1  R G1 ...
    ...

#### interpolate missing RGB data (only used by PyCraw2.c) ####

    #include"rgb.h"
    interpolateRGB_bilinear(struct cr2image *image);

### for mraw and sraw data (YCrCb) ###

#### export YUV data as TIFF ####

    #include"tiff.h"
    exportYuvTiff(&cr2, "yuv16.tiff", 0, 0); // not interpolated

Export YUV data with missing data.

Image layout for **mraw** is:

    Y1 Cb Cr, Y2 x  x, Y1 Cb Cr ... 
    Y3  x  x, Y4 x  x, Y3  x  x ...
    Y1 Cb Cr, Y2 x  x, Y1 Cb Cr ... 
    ...

**x** is missing value.

For **sraw**:

    Y1 Cb Cr, Y2  x  x, Y1 Cb Cr ... 
     x  x  x,  x  x  x,  x  x  x ...
    Y1 Cb Cr, Y2  x  x, Y1 Cb Cr ... 
    ...

To interpolate (identical to Dcraw) and export data:

    exportYuvTiff(&cr2, "yuv16i.tiff", 1, 0); // interpolate before saving, unsliced is modified

To export data in a packed format.
​    
​    exportYuvTiff(&cr2, "yuv16p.tiff", 0, 1); // packed, unsliced not modified, but interpolated data are removed by packing 

Image layout for **mraw** is:

    Y1 Cb Cr, Y2, Y1 Cb Cr ... 
    Y3, Y4, Y3, Y4 ...
    ...

For **sraw**:

    Y1 Cb Cr, Y2, Y1 Cb Cr ... 
    (no data for even lines)
    Y1 Cb Cr, Y2, Y1 Cb Cr ... 


the resulting TIFF is non standard.

    src/craw2tool.exe -e yuv16p pics/IMG_0590_mraw.CR2
    
    $ exiftool.exe -H -u yuv16p.tiff
    0x0100 Image Width : 3888
    0x0101 Image Height: 2592
    0x0010 Exif 0x0010 : 2147484295
    0x4998 Exif 0x4998 : 521 1170 1170 700
    0x4997 Exif 0x4997 : 2300 1024 1024 1711
    0x4999 Exif 0x4999 : 131074
    0x0102 Bits Per Sample : 15 15 15
    0x0103 Compression : Uncompressed
    0x0106 Photometric Interpretation  : YCbCr
    0x010a Fill Order  : Normal
    0x011c Planar Configuration: Chunky
    0x0212 Y Cb Cr Sub Sampling: YCbCr4:1:1 (4 1)
    0x013e White Point : 0.3127 0.329
    0x013f Primary Chromaticities  : 0.64 0.33 0.3 0.6 0.15 0.06
    0x0111 Strip Offsets   : 8
    0x0117 Strip Byte Counts   : 30233088
    0x0115 Samples Per Pixel   : 3
    0x0116 Rows Per Strip  : 1
    0x011a X Resolution: 300
    0x011b Y Resolution: 300
    0x0128 Resolution Unit : inches
    0x0213 Y Cb Cr Positioning : Centered
    0x0007 Exif 0x0007 : Firmware Version 1.0.5
    0x0131 Software: libcraw2 v0.7

Non standard tags are:

- Exif 0x0010 = model id
- Exif 0x4998 = RAW WB ratio
- Exif 0x4997 = SRAW WB ratio
- Exif 0x4999 = jpeg HSF and VSF
- Exif 0x0007 = Firmware version

so that unpacking and rendering is possible after loading such file using:

    yuvTiffOpen(&cr2b, "yuv16p.tiff"); // unpacked automatically using yuvUnPack()
    yuvInterpolate(&cr2b);
    ...

#### render YUV to RGB ####

    #include"yuv.h"
    struct cr2image image;
    
    cr2ImageNew( &image );
    image.isRGGB = 0; // not RGGB, is YUV
    yuv2rgb( &cr2, &image );    
    cr2ImageFree( &image );

----------
## reslicing and recompression ##

    resliced=malloc(sizeof(short) * cr2.jpeg.rawBufferLen);
    if (!resliced) {
      fprintf(stderr, "reslice malloc error");
      exit(1);
    }

### CrawSlice() ###

    // slice cr2.unsliced[] data into resliced[]
    cRaw2Slice( &cr2, resliced );
    
    scandata=malloc( cr2.jpeg.rawBufferLen *2);
    if (!scandata) {
      fprintf(stderr, "scandata malloc error");
      exit(1);
    }

### jpegCompress() ###

    // compress resliced[] into scandata[]
    jpegCompress(&(cr2.jpeg), resliced, scandata, &compressedLen, 0, 0);
      free(resliced);
    	
    if (!patchImage) // patched data can not be identical...
        if ( checkRecompression(&cr2, compressedLen, scandata) )
        printf("re-compression OK\n");

----------

## Test (NumPy and MatplotLib required) ##

warning: pycraw2_demo.py is a tentative to render raw data into RGB, it is not even beta!

PyCraw2.c is experimental tool !

    >python pycraw2_demo.py pics\5d2.cr2
    modelId = 0x80000218
    jpeg properties: high=3804, wide=1448, ncomp=4, bits=14, hsf=1, vsf=1
    w=5792 h=3804
    Canon EOS 5D Mark II
    RGGB
    image borders= [168-5783] w=5616 [56-3799] h=3744 vshift=1
    [[1056 1114 1066 1124]
    [1116 1084 1098 1059]
    [1079 1102 1044 1118]
    [1142 1082 1109 1074]]
    [2509, 1024, 1024, 1567]
    wb multipliers= [2.4501953125, 1.0, 1.0, 1.5302734375]
                        R              G1              G2               B
    min  =             971             981             991             985
    max  =           15760           15760           15761            6416
    sum  =      3605790137      3034218384      3025406789       966379992
    count=         5256576         5256576         5256576         5256576
    avg  =       1503.0235       2211.3545       2209.6782       1817.9733
    stdev=        451.8454       1116.7484       1115.1117        760.5615
    ev   =         13.8522         13.8513         13.8504         12.4070
    1024 15600
    971 15600
    scale_mul [10.976385932373196, 4.4798003964727595, 4.4798003964727595, 6.855319552024232]
    filename=rgb_py1.tiff
    cam_xyz [[ 0.4716  0.0603 -0.083 ]
    [-0.7798  1.5474  0.248 ]
    [-0.1496  0.1937  0.6651]]
    cam_rgb [[ 0.20573217  0.20186586  0.01057044]
    [ 0.01225109  0.85735756  0.20663675]
    [-0.00764955  0.16430779  0.61898383]]
    cam_rgb_norm [[ 0.49198395  0.48273811  0.02527794]
    [ 0.01138317  0.79661903  0.1919978 ]
    [-0.00986222  0.21183455  0.79802767]]
    rgb_cam [[ 2.06819029 -1.32034057  0.25215028]
    [-0.03815428  1.36546303 -0.32730875]
    [ 0.03568716 -0.37877601  1.34308885]]
    (3744L, 5616L, 3L)
    filename=rgb_py2.tiff

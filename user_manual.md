# craw2tool user manual #
---------------------
**version 0.8**

### Recompression (auto-test: jpeg decompression, unslicing, reslicing, jpeg compression, CR2 file patch)

    $ src/craw2tool.exe -r ../cr2_samples/7dm2/Y071A1864.CR2
    re-compression OK

Original and recompressed files must be identical.

### Model info: display CR2 and images properties in CSV format (used to create cr2_database.txt)

    $ src/craw2tool.exe -m ../cr2_samples/7dm2/Y071A1864.CR2
    0x80000289, Canon EOS 7D Mark II, 14, 2784, 3708, 2, 1, 2784, 2784, 1, 1, 84, 50, 5555, 3697, 0, 0, 0, 0, 5472, 3648, 0

Format is :
modelId, modelName, jpeg_bits, jpeg_wide, jpeg_high, jpeg_comp, slices[0], slices[1], slices[2], jpeg_hsf, jpeg_vsf, sensorLeftBorder, sensorTopBorder, sensorBottomBorder, SensorRightBorder, blackMaskLeftBorder, blackMasktopBorder, blackMaskBottomBorder, blackMaskRightBorder, imageWidth, imageHeight, vShift

### picture Info: display key information on picture

    $ src/craw2tool.exe -i ../cr2_samples/7dm2/Y071A1864.CR2
    5568x3708, 14bits, 2 comp, RGGB, "Canon EOS 7D Mark II", [1, 2784, 2784]
    
    $ src/craw2tool.exe -i pics/IMG_0590_mraw.CR2
    3888x2592, 15bits, 3 comp, YUV411/mraw/sraw1, "Canon EOS 60D", [8, 1296, 1296]
    
    $ src/craw2tool.exe -i pics/IMG_0045_s90.CR2
    3744x2784, 12bits, 2 comp, RGGB, "Canon PowerShot S90", [1, 1848, 1896]

### verbose and Line: display details about lossless jpeg decompression for a given line (here line 0)

    $ src/craw2tool.exe -v 3 -l 0 pics/IMG_0045_s90.CR2
    
    scanDataOffset=0x1b605e
    l=111111110(11) d=00001111100 diff1=124, diff2=-1923 p=7d, -16259
     l=111111110(11) d=00001111100 diff1=124, diff2=-1923 p=7d, -16259
     l=1101(2) d=10 diff1=2, diff2=2 p=7f, -16257
     l=100(3) d=100 diff1=4, diff2=4 p=81, -16255
     l=11110(1) d=1 diff1=1, diff2=1 p=80, -16256
     l=11110(1) d=0 diff1=0, diff2=-1 p=80, -16256
     l=11101(0) diff1=0, diff2=0 p=80, -16256
     l=11110(1) d=1 diff1=1, diff2=1 p=81, -16255
     l=1101(2) d=01 diff1=1, diff2=-2 p=7e, -16258
     l=11110(1) d=1 diff1=1, diff2=1 p=82, -16254
     l=100(3) d=100 diff1=4, diff2=4 p=82, -16254
     l=1101(2) d=01 diff1=1, diff2=-2 p=80, -16256
     l=11110(1) d=1 diff1=1, diff2=1 p=83, -16253
     l=11110(1) d=0 diff1=0, diff2=-1 p=7f, -16257
     l=100(3) d=010 diff1=2, diff2=-5 p=7e, -16258
     l=11101(0) diff1=0, diff2=0 p=7f, -16257

### display Jpeg parsing an slicing information (verbose)

```
craw2tool.exe -v 1 pics/5d2.cr2
SensorInfo: W=5792 H=3804, L/T/R/B=168/56/5783/3799
ImageInfo: W=5616 H=3744

JPEG_DHT, length=0x42
DC, Destination= 0
 maxCodeLen=10
 nbCodePerSizes: 00010402030101010101000000000000
  number of codes of length  1 bits: 0 ( )
  number of codes of length  2 bits: 1 ( 06:00, )
  number of codes of length  3 bits: 4 ( 04:010, 08:011, 05:100, 07:101, )
  number of codes of length  4 bits: 2 ( 03:1100, 09:1101, )
  number of codes of length  5 bits: 3 ( 00:11100, 0a:11101, 02:11110, )
  number of codes of length  6 bits: 1 ( 01:111110, )
  number of codes of length  7 bits: 1 ( 0c:1111110, )
  number of codes of length  8 bits: 1 ( 0b:11111110, )
  number of codes of length  9 bits: 1 ( 0d:111111110, )
  number of codes of length 10 bits: 1 ( 0e:1111111110, )
  number of codes of length 11 bits: 0 ( )
  number of codes of length 12 bits: 0 ( )
  number of codes of length 13 bits: 0 ( )
  number of codes of length 14 bits: 0 ( )
  number of codes of length 15 bits: 0 ( )
  number of codes of length 16 bits: 0 ( )
 totalNumberOfCodes= 15
DC, Destination= 1
 maxCodeLen=10
 nbCodePerSizes: 00010402030101010101000000000000
  number of codes of length  1 bits: 0 ( )
  number of codes of length  2 bits: 1 ( 06:00, )
  number of codes of length  3 bits: 4 ( 04:010, 08:011, 05:100, 07:101, )
  number of codes of length  4 bits: 2 ( 03:1100, 09:1101, )
  number of codes of length  5 bits: 3 ( 00:11100, 0a:11101, 02:11110, )
  number of codes of length  6 bits: 1 ( 01:111110, )
  number of codes of length  7 bits: 1 ( 0c:1111110, )
  number of codes of length  8 bits: 1 ( 0b:11111110, )
  number of codes of length  9 bits: 1 ( 0d:111111110, )
  number of codes of length 10 bits: 1 ( 0e:1111111110, )
  number of codes of length 11 bits: 0 ( )
  number of codes of length 12 bits: 0 ( )
  number of codes of length 13 bits: 0 ( )
  number of codes of length 14 bits: 0 ( )
  number of codes of length 15 bits: 0 ( )
  number of codes of length 16 bits: 0 ( )
 totalNumberOfCodes= 15
JPEG_SOF3, length=0x14
 bits=14, high=3804, wide=1448, nb comp=4
  index=1, h=1, v=1, qt=0
  index=2, h=1, v=1, qt=0
  index=3, h=1, v=1, qt=0
  index=4, h=1, v=1, qt=0
JPEG_SOS, length=0xe
 ns=4
  cs1 dc0 ac0
  cs2 dc1 ac0
  cs3 dc0 ac0
  cs4 dc1 ac0
 ss=1 se=0 ah/al=0x0
slice# 0, scol=    0, ecol=  484, jpeg->rawBuffer[ j ]=0
slice# 1, scol=  484, ecol=  968, jpeg->rawBuffer[ j ]=7364544
slice# 2, scol=  968, ecol= 1448, jpeg->rawBuffer[ j ]=14729088

ImageHeight=3744 [56-3799], ImageWidth=5616 [168-5783]. vshift=1
```



### display TIFF tags

To know how many tags are there, with which type and length.
Guess are made on values: if it is an array, first values are printed.

Columns are: context (ifd#, makernote=37500 or exif=34665), offset, tag_name, tag_type, tag_length, tag_raw_value (value or pointer), tag_value (value or array/string)

    >craw2tool.exe -t 5d2.cr2
        0 0x000012   256/0x100     ushort(3)*1           5616/0x15f0, 5616 (0x15f0)
        0 0x00001e   257/0x101     ushort(3)*1           3744/0xea0, 3744 (0x0ea0)
        0 0x00002a   258/0x102     ushort(3)*3            226/0xe2, 8 8 8
        0 0x000036   259/0x103     ushort(3)*1              6/0x6, 6 (0x0006)
        0 0x000042   271/0x10f     string(2)*6            232/0xe8, Canon
        0 0x00004e   272/0x110     string(2)*21           238/0xee, Canon EOS 5D Mark II
        0 0x00005a   273/0x111      ulong(4)*1          63248/0xf710, 63248 (0x0000f710)
        0 0x000066   274/0x112     ushort(3)*1              8/0x8, 8 (0x0008)
        0 0x000072   279/0x117      ulong(4)*1        2715542/0x296f96, 2715542 (0x00296f96)
        0 0x00007e   282/0x11a  urational(5)*1            270/0x10e, 72 / 1
        0 0x00008a   283/0x11b  urational(5)*1            278/0x116, 72 / 1
        0 0x000096   296/0x128     ushort(3)*1              2/0x2, 2 (0x0002)
        0 0x0000a2   306/0x132     string(2)*20           286/0x11e, 2008:11:29 14:47:12
        0 0x0000ae   315/0x13b     string(2)*1              0/0x0, 0
        0 0x0000ba 33432/0x8298    string(2)*1              0/0x0, 0
        0 0x0000c6 34665/0x8769     ulong(4)*1            434/0x1b2, 434 (0x000001b2)
    34665 0x0001b4 33434/0x829a urational(5)*1            812/0x32c, 1 / 60
    34665 0x0001c0 33437/0x829d urational(5)*1            820/0x334, 8 / 1
    34665 0x0001cc 34850/0x8822    ushort(3)*1              3/0x3, 3 (0x0003)
    34665 0x0001d8 34855/0x8827    ushort(3)*1            800/0x320, 800 (0x0320)
    34665 0x0001e4 36864/0x9000   byteseq(7)*4      825373232/0x31323230, 31323230
    34665 0x0001f0 36867/0x9003    string(2)*20           828/0x33c, 2008:11:29 14:47:12
    34665 0x0001fc 36868/0x9004    string(2)*20           848/0x350, 2008:11:29 14:47:12
    34665 0x000208 37121/0x9101   byteseq(7)*4         197121/0x30201, 00030201
    34665 0x000214 37377/0x9201  rational(10)*1            868/0x364, 393216 / 65536
    34665 0x000220 37378/0x9202 urational(5)*1            876/0x36c, 393216 / 65536
    34665 0x00022c 37380/0x9204  rational(10)*1            884/0x374, 0 / 1
    34665 0x000238 37383/0x9207    ushort(3)*1              5/0x5, 5 (0x0005)
    34665 0x000244 37385/0x9209    ushort(3)*1             16/0x10, 16 (0x0010)
    34665 0x000250 37386/0x920a urational(5)*1            892/0x37c, 105 / 1
    34665 0x00025c 37500/0x927c   byteseq(7)*41132        900/0x384, 270001000300310000005e0500000200030004000000c005000003000300...
    37500 0x000386     1/0x1       ushort(3)*49          1374/0x55e, 98 2 0 4 0 0 0 0 0 7 ...
    37500 0x000392     2/0x2       ushort(3)*4           1472/0x5c0, 0 105 52635 63988
    37500 0x00039e     3/0x3       ushort(3)*4           1480/0x5c8, 0 0 0 0
    37500 0x0003aa     4/0x4       ushort(3)*34          1488/0x5d0, 68 0 256 128 192 192 0 0 3 0 ...
    37500 0x0003b6     6/0x6       string(2)*21          1556/0x614, Canon EOS 5D Mark II
    37500 0x0003c2     7/0x7       string(2)*24          1588/0x634, Firmware Version 1.0.6
    37500 0x0003ce     9/0x9       string(2)*32          1612/0x64c,
    37500 0x0003da    12/0xc        ulong(4)*1      230106474/0xdb7256a, 230106474 (0x0db7256a)
    37500 0x0003e6    13/0xd      byteseq(7)*1536        1644/0x66c, aaaa6838683860007778000300000000000001000006000000961a626d6d...
    37500 0x0003f2    16/0x10       ulong(4)*1      2147484184/0x80000218, 2147484184 (0x80000218)
    37500 0x0003fe    19/0x13      ushort(3)*4           3180/0xc6c, 0 159 7 112
    37500 0x00040a    21/0x15       ulong(4)*1      2684354560/0xa0000000, 2684354560 (0xa0000000)
    37500 0x000416    25/0x19      ushort(3)*1              1/0x1, 1 (0x0001)
    37500 0x000422    38/0x26      ushort(3)*48          3188/0xc74, 96 2 9 9 5616 3744 5616 3744 84 84 ...
    37500 0x00042e   131/0x83       ulong(4)*1              0/0x0, 0 (0x00000000)
    37500 0x00043a   147/0x93      ushort(3)*26          3892/0xf34, 52 0 0 0 0 0 0 0 0 0 ...
    37500 0x000446   149/0x95      string(2)*70          3944/0xf68, EF24-105mm f/4L IS USM
    37500 0x000452   150/0x96      string(2)*16          4014/0xfae, N033108
    37500 0x00045e   151/0x97     byteseq(7)*1024        4030/0xfbe, 000000000000000000000000000000000000000000000000000000000000...
    37500 0x00046a   152/0x98      ushort(3)*4           5054/0x13be, 0 0 0 0
    37500 0x000476   153/0x99       ulong(4)*96          5062/0x13c6, 384 4 1 92 7 257 1 0 258 1 ...
    37500 0x000482   154/0x9a       ulong(4)*5           5446/0x1546, 0 5616 3744 0 0
    37500 0x00048e   160/0xa0      ushort(3)*14          5466/0x155a, 28 0 3 0 0 0 0 0 65535 5200 ...
    37500 0x00049a   170/0xaa      ushort(3)*6           5494/0x1576, 12 575 1024 1024 535 0
    37500 0x0004a6   180/0xb4      ushort(3)*1              1/0x1, 1 (0x0001)
    37500 0x0004b2   208/0xd0       ulong(4)*1              0/0x0, 0 (0x00000000)
    37500 0x0004be   224/0xe0      ushort(3)*17          5506/0x1582, 34 5792 3804 1 1 168 56 5783 3799 0 ...
    37500 0x0004ca 16385/0x4001    ushort(3)*1250        5540/0x15a4, 6 727 1024 1024 325 507 1024 1024 473 365 ...
    37500 0x0004d6 16386/0x4002    ushort(3)*8520        8040/0x1f68, 17040 8195 0 0 0 1561 65535 65280 41984 1297 ...
    37500 0x0004e2 16389/0x4005   byteseq(7)*16448      25080/0x61f8, 404000000300002000000000000000000000000000000000000000000000...
    37500 0x0004ee 16392/0x4008    ushort(3)*3          41528/0xa238, 129 129 129
    37500 0x0004fa 16393/0x4009    ushort(3)*3          41534/0xa23e, 0 0 0
    37500 0x000506 16400/0x4010    string(2)*32         41540/0xa244,
    37500 0x000512 16401/0x4011   byteseq(7)*252        41572/0xa264, 000000000000000000000000000000000000000000000000000000000000...
    37500 0x00051e 16402/0x4012    string(2)*32         41824/0xa360,
    37500 0x00052a 16403/0x4013     ulong(4)*5          41856/0xa380, 20 0 0 10 0
    37500 0x000536 16405/0x4015   byteseq(7)*116        41876/0xa394, 00107400000000000000000000000000000000000019f015a00e00000000...
    37500 0x000542 16406/0x4016     ulong(4)*6          41992/0xa408, 24 0 1 0 1 0
    37500 0x00054e 16407/0x4017     ulong(4)*2          42016/0xa420, 263172 2046820352
    34665 0x000268 37510/0x9286   byteseq(7)*264        42032/0xa430, 000000000000000000000000000000000000000000000000000000000000...
    34665 0x000274 37520/0x9290    string(2)*3          13872/0x3630, 13872
    34665 0x000280 37521/0x9291    string(2)*3          13872/0x3630, 13872
    34665 0x00028c 37522/0x9292    string(2)*3          13872/0x3630, 13872
    34665 0x000298 40960/0xa000   byteseq(7)*4      808464688/0x30303130, 30303130
    34665 0x0002a4 40961/0xa001    ushort(3)*1              1/0x1, 1 (0x0001)
    34665 0x0002b0 40962/0xa002    ushort(3)*1           5616/0x15f0, 5616 (0x15f0)
    34665 0x0002bc 40963/0xa003    ushort(3)*1           3744/0xea0, 3744 (0x0ea0)
    34665 0x0002c8 40965/0xa005     ulong(4)*1          42296/0xa538, 42296 (0x0000a538)
    34665 0x0002d4 41486/0xa20e urational(5)*1          42326/0xa556, 5616000 / 1459
    34665 0x0002e0 41487/0xa20f urational(5)*1          42334/0xa55e, 3744000 / 958
    34665 0x0002ec 41488/0xa210    ushort(3)*1              2/0x2, 2 (0x0002)
    34665 0x0002f8 41985/0xa401    ushort(3)*1              0/0x0, 0 (0x0000)
    34665 0x000304 41986/0xa402    ushort(3)*1              0/0x0, 0 (0x0000)
    34665 0x000310 41987/0xa403    ushort(3)*1              0/0x0, 0 (0x0000)
    34665 0x00031c 41990/0xa406    ushort(3)*1              0/0x0, 0 (0x0000)
        0 0x0000d2 34853/0x8825     ulong(4)*1          42342/0xa566, 42342 (0x0000a566)
        1 0x00ac6a   513/0x201      ulong(4)*1          44448/0xada0, 44448 (0x0000ada0)
        1 0x00ac76   514/0x202      ulong(4)*1          18798/0x496e, 18798 (0x0000496e)
        2 0x00ac88   256/0x100     ushort(3)*1            362/0x16a, 362 (0x016a)
        2 0x00ac94   257/0x101     ushort(3)*1            234/0xea, 234 (0x00ea)
        2 0x00aca0   258/0x102     ushort(3)*3          44328/0xad28, 16 16 16
        2 0x00acac   259/0x103     ushort(3)*1              1/0x1, 1 (0x0001)
        2 0x00acb8   262/0x106     ushort(3)*1              2/0x2, 2 (0x0002)
        2 0x00acc4   273/0x111      ulong(4)*1        2778792/0x2a66a8, 2778792 (0x002a66a8)
        2 0x00acd0   277/0x115     ushort(3)*1              3/0x3, 3 (0x0003)
        2 0x00acdc   278/0x116     ushort(3)*1            234/0xea, 234 (0x00ea)
        2 0x00ace8   279/0x117      ulong(4)*1         508248/0x7c158, 508248 (0x0007c158)
        2 0x00acf4   284/0x11c     ushort(3)*1              1/0x1, 1 (0x0001)
        2 0x00ad00 50649/0xc5d9     ulong(4)*1              2/0x2, 2 (0x00000002)
        2 0x00ad0c 50885/0xc6c5     ulong(4)*1              3/0x3, 3 (0x00000003)
        2 0x00ad18 50908/0xc6dc     ulong(4)*4          44334/0xad2e, 348 232 13 2
        3 0x00ad40   259/0x103     ushort(3)*1              6/0x6, 6 (0x0006)
        3 0x00ad4c   273/0x111      ulong(4)*1        3655336/0x37c6a8, 3655336 (0x0037c6a8)
        3 0x00ad58   279/0x117      ulong(4)*1       25490081/0x184f2a1, 25490081 (0x0184f2a1)
        3 0x00ad64 50648/0xc5d8     ulong(4)*1              1/0x1, 1 (0x00000001)
        3 0x00ad70 50656/0xc5e0     ulong(4)*1              3/0x3, 3 (0x00000003)
        3 0x00ad7c 50752/0xc640    ushort(3)*3          44440/0xad98, 2 1936 1920
        3 0x00ad88 50885/0xc6c5     ulong(4)*1              1/0x1, 1 (0x00000001)

### as CSV

-c max_follow (0=do not follow pointers)

    >craw2tool.exe -c 50 pics\5d2.cr2
    0, 0x12, 256, 3, 1, 0x15f0, 147461959
    0, 0x1e, 257, 3, 1, 0xea0, 67109888
    0, 0x2a, 258, 3, 3, 0xe2, 340788878
    0, 0x36, 259, 3, 1, 0x6, 67111447
    0, 0x42, 271, 2, 6, 0xe8, Canon
    0, 0x4e, 272, 2, 21, 0xee, Canon EOS 5D Mark II
    0, 0x5a, 273, 4, 1, 0xf710, 91161600
    0, 0x66, 274, 3, 1, 0x8, 158735192
    0, 0x72, 279, 4, 1, 0x296f96, 67109888
    0, 0x7e, 282, 5, 1, 0x10e, 72 / 1
    0, 0x8a, 283, 5, 1, 0x116, 72 / 1
    0, 0x96, 296, 3, 1, 0x2, 393217513
    0, 0xa2, 306, 2, 20, 0x11e, 2008:11:29 14:47:12
    0, 0xae, 315, 2, 1, 0x0, I
    0, 0xba, 33432, 2, 1, 0x0, I
    0, 0xc6, 34665, 4, 1, 0x1b2, 73533165
    34665, 0x1b4, 33434, 5, 1, 0x32c, 1 / 60
    34665, 0x1c0, 33437, 5, 1, 0x334, 8 / 1
    34665, 0x1cc, 34850, 3, 1, 0x3, 189203554
    34665, 0x1d8, 34855, 3, 1, 0x320, 133041280
    34665, 0x1e4, 36864, 7, 4, 0x31323230, 0x31323230
    34665, 0x1f0, 36867, 2, 20, 0x33c, 2008:11:29 14:47:12
    34665, 0x1fc, 36868, 2, 20, 0x350, 2008:11:29 14:47:12
    ...

```
>craw2tool.exe -c 0 pics\5d2.cr2
0, 0x12, 256, 3, 1, 0x15f0, 147461959
0, 0x1e, 257, 3, 1, 0xea0, 67109888
0, 0x2a, 258, 3, 3, 0xe2, 340788878
0, 0x36, 259, 3, 1, 0x6, 67111447
0, 0x42, 271, 2, 6, 0xe8, 0xe8
0, 0x4e, 272, 2, 21, 0xee, 0xee
0, 0x5a, 273, 4, 1, 0xf710, 91161600
0, 0x66, 274, 3, 1, 0x8, 158735192
0, 0x72, 279, 4, 1, 0x296f96, 67109888
0, 0x7e, 282, 5, 1, 0x10e, 72 / 1
0, 0x8a, 283, 5, 1, 0x116, 72 / 1
0, 0x96, 296, 3, 1, 0x2, 393217513
0, 0xa2, 306, 2, 20, 0x11e, 0x11e
0, 0xae, 315, 2, 1, 0x0, 0x0
0, 0xba, 33432, 2, 1, 0x0, 0x0
0, 0xc6, 34665, 4, 1, 0x1b2, 73533165
34665, 0x1b4, 33434, 5, 1, 0x32c, 1 / 60
34665, 0x1c0, 33437, 5, 1, 0x334, 8 / 1
34665, 0x1cc, 34850, 3, 1, 0x3, 189203554
34665, 0x1d8, 34855, 3, 1, 0x320, 133041280
34665, 0x1e4, 36864, 7, 4, 0x31323230, 0x31323230
34665, 0x1f0, 36867, 2, 20, 0x33c, 0x33c
34665, 0x1fc, 36868, 2, 20, 0x350, 0x350
...
```



### grep TIFF tag values

* Image width (tag 256 in ifd#0):

    ```
    $ src/craw2tool.exe -g 0,256,v pics/5d2.cr2
    5616
    ```

* bits per samples (tag 258 in ifd#0)

    ```
    $ src/craw2tool.exe -g 0,258,v pics/5d2.cr2
    226
    ```

    oops, this is an array. Access must be 'a' instead of 'v'

        $ src/craw2tool.exe -g 0,258,a pics/5d2.cr2
        8 8 8

* binary dump of unknown tag #224 (0xe0) in makernote. 'b' for binary

    ```
    $ src/craw2tool.exe -g m,224,b pics/5d2.cr2|btype
    0x000000:  2200a016 dc0e0100 0100a800 38009716 "...........8...
    0x000010:  d70e0000 00000000 00000000 00000000 ................
    0x000020:  0000..
    ```

    as an array:

        $  src/craw2tool.exe -g m,224,a pics/5d2.cr2
        34 5792 3804 1 1 168 56 5783 3799 0 0 0 0 0 0 0 0
        
        34 is 17*2 (tagLen * sizeof(tagType)) = inner tag length
        5792 is sensor width
        3804 is sensor height 
        ... 

see [ExifTool Canon tags](http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/Canon.html#SensorInfo "ExifTool Canon tags")

* slices array (0xc640 (50752) in ifd#3):

    ```
    $ src/craw2tool.exe -g 3,50752,a pics/IMG_0045_s90.CR2
    1 1848 1896
    ```

### patching image

from ImageMagick:    

```
$ convert -background black  -pointsize 120 -fill white label:modified_with_libcraw2 -compress none  text.pgm
```

    $ src/craw2tool.exe -a text.pgm -o image pics/IMG_0596_sraw.CR2

patched file is called image_patched.cr2

### export uncompressed and unsliced image data

    $ src/craw2tool.exe -e uncomp,unsliced pics/5d2.cr2
    
    $ exiftool -H -a -u unsliced.tiff
         - ExifTool Version Number         : 9.03
         - File Name                       : unsliced.tiff
         - Directory                       : .
         - File Size                       : 42 MB
         - File Modification Date/Time     : 2016:06:21 22:32:30+02:00
         - File Access Date/Time           : 2016:06:21 22:32:30+02:00
         - File Permissions                : rw-rw-rw-
         - File Type                       : TIFF
         - MIME Type                       : image/tiff
         - Exif Byte Order                 : Little-endian (Intel, II)
    0x0100 Image Width                     : 5792
    0x0101 Image Height                    : 3804
    0x0102 Bits Per Sample                 : 16
    0x0103 Compression                     : Uncompressed
    0x0106 Photometric Interpretation      : BlackIsZero
    0x010a Fill Order                      : Normal
    0x011c Planar Configuration            : Chunky
    0x0111 Strip Offsets                   : 8
    0x0117 Strip Byte Counts               : 44065536
    0x0115 Samples Per Pixel               : 1
    0x011a X Resolution                    : 300
    0x011b Y Resolution                    : 300
    0x0128 Resolution Unit                 : inches
    0x0131 Software                        : libcraw2 v0.7
         - Image Size                      : 5792x3804

### export RGGB image with correct borders and dimensions

    $ src/craw2tool.exe -e rggb pics/5d2.cr2   
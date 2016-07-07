# PyCanonRaw2, 13 oct 2012
# tested with 
#  comp=2:   20D (no 0xC640), 30D, 40D, 350D, 400D, 450D, 1000D, 5D, 1Ds M3, 1D-X, 650D, 5Dm3, EOS M 
#  comp=4: 7D, 5Dm2, 50D, 60D, 1100D, 500D, 550D, 600D, 1Dm4
#  12bits, comp=2: 1Dm2 (2 diff huff tables), 1D m2N, 1Ds M2, G9, G10, G12, G15, SX1, SX50, S90, S95, S100, S110
# 1D m3(slice bug)

# python -m cProfile -s time -o profile.txt cr2_analysis.py pics/40d/IMG_2525_40d.CR2

import PyTiffParser
import PyCanonRaw2
import sys

f = open(sys.argv[1],'rb')
pic = PyTiffParser.PyTiffParser( f, False )
pic.parse()

cr2 = PyCanonRaw2.PyCanonRaw2( f, pic.tags, 1)
cr2.showSummary(1)
cr2.analyseMainImage()
cr2.decodeMainImage()
f.close()

cr2.saveRaw2Tiff( 'image.bin' ) 
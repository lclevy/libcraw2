# extract thumbnails from CR2 

import sys
import Image
import PyTiffParser

f = open(sys.argv[1], 'rb')

#parse TIFF Tags
pic = PyTiffParser.PyTiffParser( f, False )
pic.parse()

#get IFD#0 picture
ifd0_stripOffsets = pic.tags[('ifd0',0x111)][3]
ifd0_stripByteCounts = pic.tags[('ifd0',0x117)][3]
print 'IFD#0 picture at 0x%x, length= 0x%x' % (ifd0_stripOffsets, ifd0_stripByteCounts)
f.seek( ifd0_stripOffsets )

out=open("ifd0.jpg", 'wb')
out.write( f.read( ifd0_stripByteCounts ) )
out.close()

#get IFD#1 picture
ifd1_thumbnailOffset = pic.tags[('ifd1',0x201)][3]
ifd1_thumbnailLength = pic.tags[('ifd1',0x202)][3]
print 'IFD#1 picture at 0x%x, length= 0x%x' % (ifd1_thumbnailOffset, ifd1_thumbnailLength)
f.seek( ifd1_thumbnailOffset )

out=open("ifd1.jpg", 'wb')
out.write( f.read( ifd1_thumbnailLength ) )
out.close()

#get IFD#2 picture (RGB 16bits, little endian)
ifd2_w = pic.tags[('ifd2',0x100)][3]
ifd2_h = pic.tags[('ifd2',0x101)][3]
ifd2_stripOffsets = pic.tags[('ifd2',0x111)][3]
ifd2_stripByteCounts = pic.tags[('ifd2',0x117)][3]

print 'IFD#2 picture at 0x%x, length= 0x%x, %dx%d' % (ifd2_stripOffsets, ifd2_stripByteCounts, ifd2_w, ifd2_h)
f.seek( ifd2_stripOffsets )
picdata = f.read( ifd2_stripByteCounts )

out=open('ifd2.ppm', 'w')
print >>out, 'P3\n%d %d' % (ifd2_w, ifd2_h)
print >>out, '%s' % (1<<14)
for y in range(ifd2_h):
  for x in range(ifd2_w):
    for c in range(3):
      offset2 = (y*ifd2_w*2*3) + (x*2*3) + c*2
      v1 = picdata[ offset2  ]
      v2 = picdata[ offset2 +1]
      pixel = (ord(v2)<<8) | ord(v1) #pixel is in little endian order
      print >>out, '%04d ' % pixel,
  print >>out
out.close()



import PyTiffParser
from binascii import hexlify
from struct import unpack
import sys

def getLongLE(d, a):
   return unpack('<L',(d)[a:a+4])[0]
def getShortLE(d, a):
   return unpack('<H',(d)[a:a+2])[0]

f = open(sys.argv[1],'rb')
pic = PyTiffParser.PyTiffParser( f, 2 )
pic.parse()
#print pic.tags

print 'ExposureTime'
print '0x829a:', 
t = pic.tags[('exif',0x829a)]
print t,
f.seek(t[3])
val = f.read(t[2]*8)
n = getLongLE(val,0)
d = getLongLE(val,4)
print '%d/%d' % (n, d)

print 'Fnumber'
print '0x829d:', 
t = pic.tags[('exif',0x829d)]
print t,
f.seek(t[3])
val = f.read(t[2]*8)
n = getLongLE(val,0)
d = getLongLE(val,4)
print pow(2, (n/d)/2)

print 'focal:', 
t = pic.tags[('maker',2)]
print t,
f.seek(t[3])
val = f.read(4)
print getShortLE(val, 2)

print 'focus distance:', 
t = pic.tags[('maker',4)]
print t,
f.seek(t[3])
val = f.read(50)
print getShortLE(val, 36),  getShortLE(val, 38) 
print hexlify(val)

print '0x0097:', 
t = pic.tags[('maker',0x0097)]
print t
f.seek(t[3]+pic.base)
val = f.read(t[2])

f.close()
#v0: 5dm3, 450d, 550d, 7d
if ord(val[0])==0:
  print hexlify(val[:0x20])
  print '0x%08x: Version = %d' % (0 , ord(val[0]))
  print '0x%08x: LensInfo = %d' % (1 , ord(val[1]))
  print '0x%08x: AVValue = %d' % (2 , ord(val[2]))
  print '0x%08x: POValue = %d' % (3 , ord(val[3]))
  print '0x%08x: DustCount = %d' % (4 , getShortLE(val, 4) )
  dustCount=getShortLE(val, 4)
  print '0x%08x: FocalLength = %d' % (6 , getShortLE(val, 6) )
  print '0x%08x: LensID = %d' % (8 , getShortLE(val, 8) )
  w = getShortLE(val, 10)
  print '0x%08x: Width = %d (0x%x)' % (10, w, w )
  h = getShortLE(val, 12)
  print '0x%08x: Height = %d (0x%x)' % (12 , h, h)
  print '0x%08x: RAW_Width = %d' % (14 , getShortLE(val, 14) )
  print '0x%08x: RAW_Height = %d' % (16 , getShortLE(val, 16) )
  print '0x%08x: PixelPitch  : %d/1000 [um]' % (18 , getShortLE(val, 18) )
  print '0x%08x: LpfDistance : %d/1000 [mm]' % (20 , getShortLE(val, 20) )
  print '0x%08x: TopOffset = %d' % (22 , ord(val[22]))
  print '0x%08x: BottomOffset = %d' % (23 , ord(val[23]))
  print '0x%08x: LeftOffset = %d' % (24 , ord(val[24]))
  print '0x%08x: RightOffset = %d' % (25 , ord(val[25]))
  print '0x%08x: Year = %d' % (26 , ord(val[26])+1900 )
  print '0x%08x: Month = %d' % (27 , ord(val[27]))
  print '0x%08x: Day = %d' % (28 , ord(val[28]))
  print '0x%08x: Hour = %d' % (29 , ord(val[29]))
  print '0x%08x: Minutes = %d' % (30 , ord(val[30]))
  print '0x%08x: BrightDiff = %d' % (31 , ord(val[31]))
  offset = 32+2
else:
#v1: 6D  
  print hexlify(val[:0x22])
  print '0x%08x: Version = %d' % (0 , ord(val[0]))
  print '0x%08x: LensInfo = %d' % (1 , ord(val[1]))
  print '0x%08x: AVValue = %d' % (2 , ord(val[2]))
  print '0x%08x: POValue = %d' % (4 , getShortLE(val,4))
  print '0x%08x: DustCount = %d' % (6 , getShortLE(val, 6) )
  dustCount=getShortLE(val, 6)
  print '0x%08x: FocalLength = %d' % (8 , getShortLE(val, 8) )
  print '0x%08x: LensID = %d' % (10 , getShortLE(val, 10) )
  w = getShortLE(val, 12)
  print '0x%08x: Width = %d (0x%x)' % (12, w, w )
  h = getShortLE(val, 14)
  print '0x%08x: Height = %d (0x%x)' % (14 , h, h)
  print '0x%08x: RAW_Width = %d' % (16 , getShortLE(val, 16) )
  print '0x%08x: RAW_Height = %d' % (18 , getShortLE(val, 18) )
  print '0x%08x: PixelPitch  : %d/1000 [um]' % (20 , getShortLE(val, 20) )
  print '0x%08x: LpfDistance : %d/1000 [mm]' % (22 , getShortLE(val, 22) )
  print '0x%08x: TopOffset = %d' % (24 , ord(val[24]))
  print '0x%08x: BottomOffset = %d' % (25 , ord(val[5]))
  print '0x%08x: LeftOffset = %d' % (26 , ord(val[26]))
  print '0x%08x: RightOffset = %d' % (27 , ord(val[27]))
  print '0x%08x: Year = %d' % (28 , ord(val[28])+1900 )
  print '0x%08x: Month = %d' % (29 , ord(val[29]))
  print '0x%08x: Day = %d' % (30 , ord(val[30]))
  print '0x%08x: Hour = %d' % (31 , ord(val[31]))
  print '0x%08x: Minutes = %d' % (32 , ord(val[32]))
  print '0x%08x: BrightDiff = %d' % (33 , ord(val[33]))
  offset = 34+2
print "dust table at 0x%x:" % offset
for i in range(offset, offset+dustCount*6, 6):
  print hexlify(val[i:i+6]),
  print 'x=%4d y=%4d size=%d' % ( getShortLE(val, i), getShortLE(val, i+2), ord(val[i+4]) )


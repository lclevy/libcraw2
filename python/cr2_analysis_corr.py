import PyTiffParser
from binascii import hexlify
from struct import unpack
import sys

def getLongLE(d, a):
   return unpack('<L',(d)[a:a+4])[0]
def getShortLE(d, a):
   return unpack('<H',(d)[a:a+2])[0]

f = open(sys.argv[1],'rb')
pic = PyTiffParser.PyTiffParser( f, False )
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

#exiftool.exe -Canon_VignettingCorr* -U ../cr2_samples/5dm3_dust_vign_chrom_corrections/FS5A9479.JPG
print '0x4015:', 
t4015 = pic.tags[('maker',0x4015)]
print t4015,
f.seek(t4015[3])
val = f.read(t4015[2])
#print hexlify(val)
print hexlify(val[0:2]), hexlify(val[2:4]), hexlify(val[4:8]), hexlify(val[8:10]), hexlify(val[10:12]), hexlify(val[12:14]), hexlify(val[14:16]),
print hexlify(val[0x10:0x12]), hexlify(val[0x12:0x14]), hexlify(val[0x14:0x16]), hexlify(val[0x16:0x18]), hexlify(val[0x18:0x1a])
for i in range(26,38,2):
  #print getShortLE(val, i), 
  print '%4x' % getShortLE(val, i), 
print hexlify(val[38:40]), hexlify(val[40:50]), hexlify(val[50:62]), hexlify(val[62:74]), hexlify(val[74:76]),
print hexlify(val[88:98]), hexlify(val[98:100]), hexlify(val[100:110]), hexlify(val[110:])
#print getShortLE(val, 0x1c2)

print '0x4016:', 
t = pic.tags[('maker',0x4016)]
print t,
f.seek(t[3])
val = f.read(t[2]*4)
print hexlify(val)

print '0x4018:', 
t = pic.tags[('maker',0x4018)]
print t,
f.seek(t[3])
val = f.read(t[2]*4)
print hexlify(val)

print '0x4019:', 
t = pic.tags[('maker',0x4019)]
print t,
f.seek(t[3])
val = f.read(t[2])
print hexlify(val)

print '0x4021:', 
t = pic.tags[('maker',0x4021)]
print t,
f.seek(t[3])
val = f.read(t[2]*4)
print hexlify(val)

print '0x4025:', 
t = pic.tags[('maker',0x4025)]
print t,
f.seek(t[3])
val = f.read(t[2]*4)
print hexlify(val)

print '0x4027:', 
t = pic.tags[('maker',0x4027)]
print t,
f.seek(t[3])
val = f.read(t[2]*4)
print hexlify(val)

"""print '0x4028:', 
t = pic.tags[('maker',0x4028)]
print t,
f.seek(t[3])
val = f.read(t[2]*4)
print hexlify(val)"""


f.close()


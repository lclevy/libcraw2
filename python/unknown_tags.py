import PyTiffParser
from binascii import hexlify
from struct import unpack
import sys

def getLongLE(d, a):
   return unpack('<L',(d)[a:a+4])[0]
def getShortLE(d, a):
   return unpack('<H',(d)[a:a+2])[0]

def getTagLength(tags, key):   
  if key in tags:
    tag = tags[key]
    return tag[2]
  else:
    return '' 

def getTagValue(tags, key):   
  if key in tags:
    tag = tags[key]
    return tag[3]
  else:
    return ''    

def getTagData(tags, key, f):   
  if key in tags:
    tag = tags[key]
    f.seek(tag[3])
    return fread(tag[2])
  else:
    return ''
    
f = open(sys.argv[1],'rb')
pic = PyTiffParser.PyTiffParser( f, False )
pic.parse()
#print pic.tags
#pic.listTagsCsv()

#print 'modelId,',
tagList = [ 0x83, 0x97, 0x4001, 0x4002, 0x4003, 0x4004, 0x4005, 0x4008, 0x4009, 0x4010, 0x4011, 0x4012, 0x4013, 
  0x4014, 0x4015, 0x4016, 0x4017, 0x4018, 0x4019, 0x4020, 0x4021, 0x4023, 0x4024, 0x4025, 0x4027, 0x4028 ]
#tagList = range( 0x4001, 0x4029 )
"""for t in tagList:
  print '0x%x,' % t,
print  
"""
#modelId
t = pic.tags[('maker',0x10)]
#print t,
print '0x%08x,' % t[3],

#modelId
t = pic.tags[('ifd0',0x110)]
#print t,
f.seek(t[3])
val = f.read(t[2])
print '%s,' % val,

for t in tagList:
  #print '0x%x:' % t, 
  print '%s,' % getTagLength( pic.tags, ('maker',t) ),
print  
"""  
print '0x4016:', 
t = pic.tags[('maker',0x4016)]
print t,
f.seek(t[3])
val = f.read(t[2]*4)
print hexlify(val)
"""

f.close()


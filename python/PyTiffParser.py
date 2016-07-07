"""
PyTiffParser,
a parser in python for TIFF/Canon metadata analysis (parses jpeg and .CR2)

version 0.1 (16jun2012) 
do not take it as a reference parser, even if it does the job most of the time!

Laurent Clevy
http://lclevy.free.fr/cr2
"""

from struct import unpack
import os

JPEG_SOI = 0xffd8
JPEG_APP1 = 0xffe1

INTEL_BYTE_ORDER    = 0x4949
MOTOROLA_BYTE_ORDER = 0x4d4d

TIFFTAG_VAL_MAKERNOTE        = 0x927c
TIFFTAG_VAL_EXIF             = 0x8769
TIFFTAG_VAL_EXIF_USERCOMMENT = 0x9286

def getLongLE( d, a):
   return unpack('<L',(d)[a:a+4])[0]
def getLongBE(d, a):
   return unpack('>L',(d)[a:a+4])[0]
def getShortLE(d, a):
   return unpack('<H',(d)[a:a+2])[0]
def getShortBE(d, a):
   return unpack('>H',(d)[a:a+2])[0]


class PyTiffParser:

   def __init__(self, f, verbose=0):
      self.tags = dict()
      self.file = f
      self.verbose = verbose    
      self.order, self.base, self.type = self.checkHeader( verbose )
      print self.base
      
   def dumpTag( self, offset, len ):
      old_pos = self.file.tell()
      read = 0
      self.file.seek(offset+self.base)
      print "0x%06x:" % (self.file.tell()),
      line = min (16, len-read)
      d = self.file.read( line )
      read = read + line
      while (len-read) >0:
        for i in range(line):
            print "%x" % (ord(d[i])),
        for j in range(16-line):
            print " "
        for i in range(line):
            print "%x" % (ord(d[i])),
        print
        line = min (16, len-read)
        d = self.file.read( line )
        read = read + line
      self.file.seek(old_pos)
      return

   def fGetLong( self, order ):
      m = self.file.read(4)
      if order==INTEL_BYTE_ORDER:
         return getLongLE(m, 0)
      else:
         return getLongBE(m, 0)
   
   def fGetShort( self, order ):
      m = self.file.read(2)
      if order==INTEL_BYTE_ORDER:
         return getShortLE(m, 0)
      else:
         return getShortBE(m, 0)
   
   def parseMakernote( self, offset ):
      old_pos = self.file.tell()
      self.file.seek(offset + self.base)
      if self.verbose>0:
         print 'maker base=%d' % self.base
      n = self.fGetShort( self.order )
      if self.verbose>0:
         print "\n Makernote entries = %d" % (n)
      for i in range(n):
         tag = self.fGetShort( self.order )
         type = self.fGetShort( self.order )
         length = self.fGetLong( self.order )
         val = self.fGetLong( self.order )
         self.tags[('maker', tag)] = (tag, type, length, val)
         if self.verbose>0:
            print "tag = 0x%x/%d, type = %d, length = 0x%x/%d, val = 0x%x/%d" % (tag,tag,type,length,length,val,val)
         """if (tag==0x4001) and self.verbose>1:
            print "tag = 0x%x/%d, type = %d, length = 0x%x/%d, val = 0x%x/%d" % (tag,tag,type,length,length,val,val)
            self.dumpTag( val, length)"""
      self.file.seek(old_pos)
      if self.verbose>0:
         print "end of Makernote\n"
      return
   
   def parseExif( self, offset ):
      old_pos = self.file.tell()
      self.file.seek(offset + self.base)
      n = self.fGetShort( self.order )
      if self.verbose>0:
         print "\n Exif entries = %d" % (n)
      for i in range(n):
         tag = self.fGetShort( self.order )
         type = self.fGetShort( self.order )
         length = self.fGetLong( self.order )
         val = self.fGetLong( self.order )
         if self.verbose>0:
            print "tag = 0x%x/%d, type = %d, length = 0x%x/%d, val = 0x%x/%d" % (tag,tag,type,length,length,val,val)
         self.tags[('exif', tag)] = (tag, type, length, val)
         if tag == TIFFTAG_VAL_MAKERNOTE:
            self.parseMakernote( val )
      self.file.seek(old_pos)
      if self.verbose>0:
         print "end of Exif\n"
      return
   
   def parseIFD( self, currentIfd ):
      n = self.fGetShort( self.order )
      for i in range(n):
         tag = self.fGetShort( self.order )
         type = self.fGetShort( self.order )
         length = self.fGetLong( self.order )
         val = self.fGetLong( self.order )
         if self.verbose>0:
            print "tag = 0x%x/%d, type = %d, length = 0x%x/%d, val = 0x%x/%d" % (tag,tag,type,length,length,val,val)
         ifdName = 'ifd{0:x}'.format(currentIfd)
         self.tags[(ifdName, tag)] = (tag, type, length, val)
         if tag == TIFFTAG_VAL_EXIF:
            self.parseExif( val )
      return self.fGetLong( self.order )
      
   def parse( self ):
      nIfd = 0
      offset = 1
      while offset != 0:
        if self.verbose>0:
           print "IFD#%d" % (nIfd)
        offset = self.parseIFD( nIfd )
        if offset!=0:
           self.file.seek(self.base+offset)
        nIfd = nIfd + 1
      self.nIfd = nIfd
      return
   
   def checkHeader( self, verbose=0 ):
      """
      check picture header and determine if it is jpeg or tiff
      returns order (intel or motorola) and base     
      """
      self.file.seek(0, 0) #beginning of file
      m = self.file.read(2)
      if getShortBE(m, 0)==JPEG_SOI:     #jpeg
         if self.verbose>0:
            print "jpeg"
         if self.fGetShort( MOTOROLA_BYTE_ORDER ) == JPEG_APP1:
            print "app1"
            size = self.fGetShort( MOTOROLA_BYTE_ORDER )
            print "size=0x%x/%d" % (size, size)
            self.file.read(6)
            m = self.file.read(2)
            order = getShortBE(m, 0)
            base = self.file.tell()-2
            if self.verbose>1:
               print "base = 0x%x" % (base)
            magic = self.fGetShort( order )
            offset = self.fGetLong( order )
            if self.verbose>1:
               print "offset = 0x%x" % (offset)
            self.file.seek(offset+base)
            return order, base, 'jpeg'
      else:  #TIFF/CR2
         order = getShortBE(m, 0)
         if order==INTEL_BYTE_ORDER or order==MOTOROLA_BYTE_ORDER:
            if self.verbose>0:
               print "TIFF, Byte order= %s" % (m)
            base = 0
            magic = self.fGetShort( order )
   #   print "magic = 0x%x" % (magic)
            offset = self.fGetLong( order )
   #         print "offset = 0x%x" % (offset)
   
            self.file.seek(offset+base)
            return order, base, 'tiff'
      return -1, -1, 'unknown'

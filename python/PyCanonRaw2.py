# PyCanonRaw2  14oct2012
# Laurent Clevy (http://lclevy.free.fr/cr2)
# tested with 
#  comp=2: 20D (no 0xC640), 30D, 40D, 350D, 400D, 450D, 1000D, 5D, 1Ds M3, 1D-X, 650D, 5Dm3, EOS M 
#  comp=4: 7D, 5Dm2, 50D, 60D, 1100D, 500D, 550D, 600D, 1Dm4
#  12bits, comp=2: 1Dm2 (2 diff huff tables), 1D m2N, 1Ds M2, G9, G10, G12, G15, SX1, SX50, S90, S95, S100, S110
# 1D m3(slice bug)
# decompression + unslicing output is identical to dcraw (and written from scratch)
# todo: sraw, scaling and basic demosaicing, C functions for decompression & unslicing

from struct import unpack
from array import array
from binascii import hexlify
import os
import Image

JPEG_SOI  =      0xffd8
JPEG_APP1 =      0xffe1
JPEG_DHT  =      0xffc4
JPEG_DQT  =      0xffdb
JPEG_SOF0 =      0xffc0
JPEG_SOF3 =      0xffc3
JPEG_SOS  =      0xffda
JPEG_EOI  =      0xffd9

bitmask = dict()

mask = 0
for i in range(1,32):
  mask = mask|1
  bitmask[i] = mask
  mask = mask<<1

def getShortBE( m, o ):
   return unpack('<H',m[o:o+2])[0]
def getShortLE( m, o ):
   return unpack('>H',m[o:o+2])[0]

def binaryPrint(val, length):
   string = ''
   for i in range(length):
      if val&1:
         string = '1'+string
      else:
         string = '0'+string
      val = val >>1     
   return string

class PyCanonRaw2:
   def __init__(self, f, tags, verbose=0):      
      self.file = f
      self.tags = tags
      self.verbose = verbose
      self.huffTrees = [] 
      self.huffTables = []
      self.dcTableN = []
      self.hv = []
      self.ncomp = 0
      self.high = 0
      self.wide = 0
      self.bits = 0
      self.scandataOffset = 0
      self.jpg = ''
      self.slices = []
      self.image = ''
      self.bitsBuffer = 0
      self.bufferLength = 0
      self.dataOffset = 0

   def showSummary(self, level=0):
      if ('maker',0xe0) in self.tags:
         sensorOffset = self.tags[('maker',0xe0)][3]
         self.file.seek(sensorOffset)
         sensor = self.file.read(32)
         sensorData = dict()
         sensorDataNames = ['SensorWidth', 'SensorHeight', '?', '?', 'SensorLeftBorder', 'SensorTopBorder', 'SensorRightBorder', 	 
            'SensorBottomBorder', 'BlackMaskLeftBorder', 'BlackMaskTopBorder', 'BlackMaskRightBorder', 'BlackMaskBottomBorder' ] 	 
         print 'SensorInfo =',
         for i in range(1, 13):
            sensorData[ i ] = getShortBE(sensor, i*2)  
            print ' %s=%d' % (sensorDataNames[i-1], sensorData[i] ),
         print '\n'
      
         print 'Tags 0x40xx sizes\n ',
         for tagNum in [ 0x4001, 0x4002, 0x4005 ]: 
            if ('maker',tagNum) in self.tags:
               print '0x%x: %d' % ( tagNum, self.tags[('maker',tagNum)][2] ),
         print '\n'

      # like exiftool -Model
      modelOffset = self.tags[('ifd0',272)][3]
      self.file.seek(modelOffset)
      model = self.file.read(32)
      print 'Model =', model


   def analyzeDHT(self, offset ):
      if self.verbose>0:
         if ord(self.jpg[offset])&0xf0 !=0 :
            print ' AC,',
         else:
            print ' DC,',
         print 'Destination= %d' % (ord(self.jpg[offset]) & 0xf)      
      # http://www.w3.org/Graphics/JPEG/itu-t81.pdf, Annex C, page 50   
      nbCodePerSizes = self.jpg[offset+1:offset+1+16]
      if self.verbose>0:
         print '  nbCodePerSizes: ' + hexlify(nbCodePerSizes)
      nbCodePerSizesTable = dict()
      nbValues = 0
      valuesPerLength=[]
      for i in range(16):
         if self.verbose>0:
            print '  number of codes of length %2d bits: %d (' % (i+1, ord(nbCodePerSizes[i]) ), 
         nbCodePerSizesTable[i+1] = ord(nbCodePerSizes[i])
         valuesForThisLength= []
         for j in range(nbCodePerSizesTable[i+1]):
            if self.verbose>0:
               print '%02x' % ord(self.jpg[offset+1+16+j+nbValues]),
            valuesForThisLength.append( ord(self.jpg[offset+1+16+j+nbValues]) )
         if self.verbose>0:
            print ')'
         valuesPerLength.append( valuesForThisLength ) 
         nbValues = nbValues + nbCodePerSizesTable[i+1]
      if self.verbose>0:
         print ' totalNumberOfCodes= %d' % nbValues    
      symbolsPerCode = self.jpg[offset+1+16:offset+1+16+nbValues]
      if self.verbose>0:
         print ' symbolsPerCode: ' + hexlify(symbolsPerCode)
      return valuesPerLength, nbValues  

   # http://www.w3.org/Graphics/JPEG/itu-t81.pdf, section B.2.4.2, and Annex C
   def generateHuffmanTree( self, values ):
      codeLen = 1
      code = 0
      tree = []
      maxCodeLen = 0
      for length2 in values:  #walkthrough the bits length list 
         for value in length2:  #walkthrough all codes of this length
            if self.verbose>0:
               print "%2d %2d %04x %s" % (value, codeLen, code, binaryPrint(code, codeLen))
            tree.append([value,codeLen,code])
            maxCodeLen = codeLen #order by increasing length, so the latest is the max one
            code+=1   #increment code
         codeLen+=1  #increment code length
         code<<=1    #add one 0 at right of the code 
      return tree, maxCodeLen

   #easy to understand, but slow
   def findHuffCode(self, tree, val, valLength): 
     codeLen =-1
     for code in tree:
        if (val>>(valLength-code[1])) == code[2]:  #test HufCode (code[2]) of length in code[1]
           #print 'val=%d, len=%d' % (code[0], code[1])
           codeLen = code[1]
           huffVal = code[0]
     if codeLen<0:
        print "error: no huffCode found for %x" % val
     return codeLen, huffVal

   #using look up table
   #See http://w3-o.cs.hm.edu/~ruckert/understanding_mp3/hc.pdf, pages 156 and 157
   def createHuffTable( self, tree, maxCodeLen ):
      table = array('L', [0]* (1<<maxCodeLen) )
      prevIndex = 0
      #print 'maxCodeLen=%d' % maxCodeLen
      for code in tree:
         #print 'codeLen=%2d codeVal=%-15s' % ( code[1], binaryPrint(code[2], code[1]) ), 
         suffix = 0
         prefix = code[2]<<(maxCodeLen-code[1])
         indexRange = 1<<(maxCodeLen-code[1])
         #print 'prefix=%x prevIndex=%d endIndex=%d val=%x' % ( prefix, prevIndex, indexRange+prevIndex, code[2]<<4 | code[1] )
         for i in range(prevIndex, prevIndex+indexRange ):
            table[ prefix | suffix ] = code[0]<<4 | code[1]  #val on 28 MSB, length on 4 LSB. max length = 15 < 2^4
            suffix = suffix + 1
         prevIndex = prevIndex+indexRange  
      return table

   #using a table this time = 3 times faster, but use a table of 2^maxCodeLen longs
   def findHuffCode2(self, table, val, valLength, maxCodeLen):
      #print 'val=%x valLength=%d' % (val, valLength),
      if valLength>maxCodeLen:
         val = val>>(valLength-maxCodeLen) 
      #print 'val2=%d ' % (val)
      entry = table[ val ]
      codeLen = entry & 0xF    #val on 28 MSB, length on 4 LSB. max length = 15 < 2^4
      huffVal = entry >>4  
      return codeLen, huffVal

   #read bits from jpeg lossless huffmann data
   #bitsBuffer is filled with bufferLength bits, which are right justified
   #bufferLength is kept>=16 until dataSize == column
   #data points after Jpeg SOF3 header 
   def getBits(self, data, dataSize):
      while  self.dataOffset < dataSize and ( self.bufferLength < 16 ):
         byteVal = ord(data[self.dataOffset])
         self.bitsBuffer = self.bitsBuffer<<8 | byteVal
         self.dataOffset = self.dataOffset+1
         self.bufferLength = self.bufferLength+8
         if byteVal==0xff:        #byte sequence ff00 means ff 
            if ord(data[self.dataOffset])==0xD9:
               #print '0x%X marker! dataOffset=%d' % ( JPEG_EOI, dataOffset)
               return self.bitsBuffer, self.bufferLength
            self.dataOffset = self.dataOffset+1
      #print "bitsBuffer=%02x/%s, bufferLength=%d, dataOffset=%d" % (bitsBuffer, binaryPrint(bitsBuffer, bufferLength), bufferLength, dataOffset)
      return self.bitsBuffer, self.bufferLength
    
   #remove 'length' left most bits from bitsBuffer
   def shiftBits(self, length, data, dataSize):
      if self.bufferLength<length:
         getBits(data, dataSize)
         print 'shiftBits feeding, should not happen'
      self.bufferLength = self.bufferLength-length
      self.bitsBuffer = self.bitsBuffer & bitmask[self.bufferLength]   
 
   
 
   #decode Lossless Sequential Huffman stream. See itu-t81.pdf, Annex H, page 132
   def decodeRaw( self ):
      global dataOffset
      data = self.jpg[self.scandataOffset:]
      size = 0
      for hv in self.hv:
        size = size + hv[0]*hv[1]   #number of picture elements, we could have h*v = 1*1, 2*1, 2*2
      print "size=%d" % size  
      rowSize = size * self.wide
      raw = array('H', [0] *(rowSize * self.high) )
   #  currentLine = array('H', [0] *(wide*ncomp) ) #optimize: memory with only two raw line before "cr2 deslicing"
   #  prevLine = array('H', [0] *(wide*ncomp) )
      imageOffset = 0
      print range(1)
      if self.verbose>0:
         print "decoding line",
      #print 'rowSize=%d' % rowSize
      for line in range(self.high):
         if self.verbose>0 and ((line%100)==0 or line==self.high-1):
            print line,
         #decompress one line    
         for column in range(self.wide):
           for comp in range(len(self.hv)):
              compSize = self.hv[comp][0]*self.hv[comp][1]
              for comp2 in range(compSize):
                 print 'col=%d comp=%d %d' % (column, comp, comp2),            
                 bitsBuffer, bufferLength = self.getBits(data, self.stripByteCount - self.scandataOffset)
                 #huffman encoded length of the following Diff value
                 codeLen, huffVal = self.findHuffCode2(self.huffTables[ self.dcTableN[comp] ][0], bitsBuffer, bufferLength, 
                   self.huffTables[ self.dcTableN[comp] ][1])
                 #codeLen, huffVal = findHuffCode(self.huffTrees[column%2], bitsBuffer, bufferLength) #tree[0] for even columns, tree[1] for odd ones
                 self.shiftBits(codeLen, data, rowSize) #remove length bits
                 #if line==0:
                 #  print "codeLen=%d, huffVal=%d" % ( codeLen, huffVal ),
                 if huffVal==0:  #length of 0 bits means diff = 0
                    diffVal = 0
                 else:       #read huffVal bits of Diff value
                    bitsBuffer, bufferLength = self.getBits(data, self.stripByteCount - self.scandataOffset) 
                    #if line==0:
                    #  print "bitsBuffer=%x, bufferLength=%d" % ( bitsBuffer, bufferLength ),
                    diffVal = ( bitsBuffer>>(bufferLength-huffVal) ) & bitmask[huffVal]
                    self.shiftBits(huffVal, data, rowSize)
                    #if line==0:
                    #  print "diff%d=%x" % ( column%2, diffVal ),
                    #see Table H.2 in itu-t81.pdf  
                    if ( diffVal & ( 1<<(huffVal-1) ) ) == 0: #if MSB is 0, diff is positive, else negative
                       diffVal = - ( diffVal ^ bitmask[huffVal] )   #dcraw does "diff -= (1 << len) - 1"
                 print "diffVal2=%d " % ( diffVal ),
                 if line<1:
                    if column < compSize:
                       pixel = diffVal + (1<<(self.bits-1))  #default predicted value
                       raw[rowSize*line + column] = pixel
                 else: #line>=1
                    if column < self.ncomp:
                       pixel = raw[rowSize*(line-1) + column] + diffVal
                       raw[rowSize*line + column] = pixel
                 if column >= self.ncomp:
                    pixel = raw[rowSize*line + column-self.ncomp ] + diffVal  
                    raw[rowSize*line + column] = pixel 
                 if self.verbose>0 and (line==2621 or line==0):
                   print 'pix=%x' % ( pixel  )
                 #if line==0:
                   #print "\n"
      f = open("raw.bin", "wb")    #for testing
      f.write(raw)
      f.close()
      if self.verbose>0:
         print "\n"
      return raw
   
   def unslice(self, raw):
      rowSize = self.ncomp * self.wide          
      image = array('H', [0] *(rowSize * self.high) )
      if self.slices == []: #for the 20D
         self.slices = [0, 0, rowSize]
      #fills image buffer (vertical) slices per slices
      dstCol = dstLine = 0
      currentSlice = 0
      if self.verbose>0:
         print 'decoding slice ',
      for srcLine in range(self.high):
         for srcCol in range(rowSize):        
            image[ dstLine*rowSize + dstCol ] = raw[ srcLine*rowSize + srcCol ]
            if currentSlice < self.slices[0]:
               if dstCol < ( ((currentSlice+1)*self.slices[1])-1 ) :
                  dstCol = dstCol+1
               else: #end of line in the slice
                  if dstLine < self.high-1:
                     dstLine = dstLine+1
                  else: #last line in the slice
                     if self.verbose>0:
                        print currentSlice,
                     dstLine = 0
                     currentSlice = currentSlice+1
                  dstCol = currentSlice*self.slices[1]
            else: #last slice
               #if dstLine>2618:
               #  print "=dstLine=%d dstCol=%d currentSlice=%d srcLine=%d srcCol=%d raw=%x, " % (dstLine, dstCol, currentSlice, srcLine, srcCol, raw[ srcLine*rowSize + srcCol ])
               if dstCol < rowSize-1:    
                  dstCol = dstCol+1
               else:
                  dstCol = currentSlice*self.slices[1]
                  if dstLine < self.high:
                     dstLine = dstLine+1
                  else:
                     print 'dstLine == high-1'
      if self.verbose>0:
         print currentSlice
      return image      

   def analyseMainImage(self):           
      if ('ifd3',0x111) in self.tags:
         self.stripOffset = self.tags[('ifd3',0x111)][3]
         self.stripByteCount = self.tags[('ifd3',0x117)][3]
         #read jpeg data
         self.file.seek(self.stripOffset)
         self.jpg = self.file.read(self.stripByteCount)
         #get slice information
         self.slices = []
      if ('ifd3',0xc640) in self.tags: #no such tag in 20D
         cr2SliceOffset = self.tags[('ifd3',0xc640)][3]
         self.file.seek(cr2SliceOffset)
         slice = self.file.read(2*3)
         for i in range(3):
            self.slices.append( getShortBE(slice,i*2) ) 
         if self.verbose>0:
            print "cr2Slice = ", self.slices
   
      val = getShortLE(self.jpg, 0)
      if val != JPEG_SOI:
         print 'no JPEG_SOI'
         sys.exit()

      offset = 2
      val = getShortLE(self.jpg, offset)
      while val != JPEG_SOS:
     #   print '0x%04X' % val
         if val == JPEG_APP1:
            print 'JPEG_APP1:'
         elif val == JPEG_DHT:
            print 'JPEG_DHT:', 
            length = getShortLE(self.jpg, offset+2)
            print 'length = 0x%x,' % length 
            localOffset = offset+4
            for i in range(2):  #2 components
              valuesPerLength, nbValues = self.analyzeDHT( localOffset )
              tree, maxCodeLen = self.generateHuffmanTree( valuesPerLength )
              self.huffTrees.append( tree ) # in case we would use findHuffCode
              table = self.createHuffTable( tree, maxCodeLen )  #maxCodeLen is 15 for jpeg
              self.huffTables.append( [table, maxCodeLen] )
              localOffset = localOffset+1+16+nbValues
               
         elif val == JPEG_DQT:
            print 'JPEG_DQT:',
            length = getShortLE(self.jpg, offset+2)
            print 'length = 0x%02x' % length
         elif val == JPEG_SOF0 or val == JPEG_SOF3:
            print 'JPEG_SOF%d:' % (val&0xf),
            length = getShortLE(self.jpg, offset+2)
            print 'length = 0x%x' % length
            self.bits = ord(self.jpg[offset+4])
            self.high = getShortLE(self.jpg, offset+5)
            self.wide = getShortLE(self.jpg, offset+7)
            self.ncomp = ord(self.jpg[offset+9])
            print ' bits=%d, high=%d, wide=%d, nb comp=%d' % (self.bits, self.high, self.wide, self.ncomp)
            for comp in range(self.ncomp):
              i = ord(self.jpg[offset+10+comp*3])
              hv = ord(self.jpg[offset+11+comp*3])
              qt = ord(self.jpg[offset+12+comp*3])
              print '  index=%d, h=%d, v=%d, qt=%d' % (i, hv>>4, hv & 0xf, qt)
              self.hv.append( [ hv>>4, hv & 0xf ] ) 
         else:
            print '? 0x%4x' % val
            length = 2
         offset += length+2 
         val = getShortLE(self.jpg, offset)
         print self.hv
      
      print 'JPEG_SOS:',
      length = getShortLE(self.jpg, offset+2)
      print 'length = 0x%02x' % length
      ns = ord(self.jpg[offset+4])
      print ' ns=%d' % ns
      for i in range(ns):
         print '  cs%d dc%d ac%d' % ( ord(self.jpg[offset+5+i*2]), ord(self.jpg[offset+5+i*2+1])>>4, 
           ord(self.jpg[offset+5+i*2+1])&0xf )
         self.dcTableN.append(ord(self.jpg[offset+5+i*2+1])>>4)  
      print ' ss=%d' % ord(self.jpg[offset+5+(ns*2)])
      print ' se=%d' % ord(self.jpg[offset+5+(ns*2)+1])
      print ' ah/al=0x%02x' % ord(self.jpg[offset+5+(ns*2)+2])
      self.scandataOffset = offset+5+(ns*2)+3
      print self.dcTableN

   def decodeMainImage(self):
      slicedRaw = self.decodeRaw()
      #f = open("raw.bin", "rb")
      #slicedRaw = array( 'H', f.read() )
      #f.close()
      self.image = self.unslice(slicedRaw)
      f = open("image.bin", "wb")
      f.write(self.image)
      f.close()
      return self.image
      
   def saveRaw2Tiff(self, filename):
      im = Image.frombuffer('I;16',(self.wide*self.ncomp, self.high), self.image, "raw", 'I;16',0, 1)
      im.save('image.tiff')  
   
  
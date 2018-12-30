# to render RAW using PyCraw2 extension and NumPy
# "PyCraw2 demo" 
# Laurent Clevy, @lorenzo2472,  http://lclevy.free.fr/cr2
# tested with NumPy 1.15, Python 3.6


from craw2 import craw2, INTERPOLATION_BILINEAR #import PyCraw2
import sys
import array
import string
import numpy as np
#import pylab as pl #to draw histograms

#to get model name from IDF#0, tag #0x110.
#to show craw2.getTagContent() usage
def getModelName(cr2, tags):
  #get TIFF tag for model string from IFD#0, id 0x110. See http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/EXIF.html
  _id, _type, _len, value, offset = tags["0x0_0x110"] # ifd#0 0x110

  r = cr2.getTagContent( (_type, _len, value), 'a')
  r2 = [chr(x) for x in r]
  return "".join(r2)

#retrieve rendering constants from DNG database (which is automatically generated using dng_info.sh script)
def getDNGInfo(dng_info):  
  f = open('dng_info.txt')
  for l in f:
    id, model, dark, sat, matrix = l.split(',')
    # float for dark because of PowerShot G1 X Mark II = 511.46875, PowerShot S95 = 128.1054688 ...
    # if id == 0x80000289y, key will be str(int(0x80000289,16)) + 'y'
    # r for rggb values, y for YCbCr values
    dng_info[ str(int(id[:-1],16))+id[-1] ] = (model[1:], [int(np.ceil(float(x))) for x in dark.split()], [int(x) for x in sat.split()], [float(x) for x in matrix.split()])  
  f.close()

#generate basic histogram for RGGB and YCbCr
#just a try, could be really improved with better PyPlot skills...  
def histogram(colors, ev, compNames):
  hist = [np.histogram( x, bins=np.floor(ev*48) ) for x in colors] #TypeError: `bins` must be an integer, a string, or an array
  if len(colors)==4: #RGGB
    fig, (ax1, ax2, ax3, ax4) = pl.subplots(len(colors),1, sharex=True)
    plotlist = zip( (ax1, ax2, ax3, ax4), compNames)
  else: #YCbCr
    fig, (ax1, ax2, ax3) = pl.subplots(len(colors),1, sharex=True)
    plotlist = zip( (ax1, ax2, ax3), compNames)
  i=0
  for (p,c) in plotlist:
    x = hist[i][1][:-1]
    y = hist[i][0]
    if len(colors)==4: #RGGB
      p.fill_between(x, 0, y, facecolor=c)
    else:      
      p.fill_between(x, 0, y, facecolor='black')
    p.plot(x, y, c[0] )
    i=i+1
  pl.suptitle(sys.argv[1]+' ('+modelName[:-1]+')')  
  pl.savefig("histo.pdf")
  return

def computeColorMatrix( modelId ):  
  #camera color matrix (from DNG converted file, Tag ColorMatrix2 (0xC622)), in XYZ color space
  m = dng_info[ modelId ][3] #color matrix
  if m==None: #no DNG info for this camera
    return None
  cam_xyz = np.reshape( m, (3,3) )
  print ("cam_xyz", cam_xyz ) 
  #conversion to RGB color space
  xyz_rgb = [ [ 0.412453, 0.357580, 0.180423 ], [ 0.212671, 0.715160, 0.072169 ], [ 0.019334, 0.119193, 0.950227 ] ] 
  cam_rgb = np.matrix(cam_xyz) * np.matrix(xyz_rgb)
  print ("cam_rgb", cam_rgb)
  #normalization
  rows_sums = cam_rgb.sum(axis=1)
  cam_rgb_norm = cam_rgb / rows_sums
  print ("cam_rgb_norm", cam_rgb_norm)
  #matrix to convert camera color space to sRGB
  rgb_cam = cam_rgb_norm.I     #inverse matrix
  print ("rgb_cam", rgb_cam)
  return rgb_cam
 
def stats(colors, counts):  
  #stats
  mins = [np.amin(x) for x in colors] 
  maxs = [np.amax(x) for x in colors] 
  if len(colors)==4:
    print ('                    R              G1              G2               B')
  else:
    print ('                    Y              Cb              Cr')
  print ('min  = '+len(colors)*'%15d ' % tuple(mins))
  print ('max  = '+len(colors)*'%15d ' % tuple(maxs))
  sums = [np.sum(x) for x in colors]
  print ('sum  = '+len(colors)*'%15d ' % tuple(sums))
  #seems a method to automatically find wb RGGB ratio, based on the hypothesis that on average RGGB proportion is equal
  #ratio = [float(counts[x])/float(sums[x]) for x in range(4)]
  #print 'ratio= %15.4f %15.4f %15.4f %15.4f' % tuple(ratio)
  print ('count= '+len(colors)*'%15d ' % tuple(counts))
  avg = [np.average(x) for x in colors]
  print ('avg  = '+len(colors)*'%15.4f ' % tuple(avg))
  stdev = [np.std(x) for x in colors]
  print ('stdev= '+len(colors)*'%15.4f ' % tuple(stdev))
  evs = [ np.log2(maxs[x]-mins[x]) for x in range(len(colors)) ]
  print ('ev   = '+len(colors)*'%15.4f ' % tuple(evs))

if len(sys.argv)<2:
  print ("missing CR2 file")	
  exit(1)

#retrieve DNG info. Used for linearisation (min and max) and sRGB conversion (colors matrix)
dng_info = dict()  
getDNGInfo( dng_info )

  
#create object
cr2 = craw2()
cr2.open(sys.argv[1])

#retrieve TIFF tags
tags = cr2.getTiffTags()
#print tags
#get tag 0x10 in MakerNote = Model ID
_id, _type, _len, modelId, offset = tags["0x927c_0x10"] #makernote 0x10
print( "modelId = 0x%08x" % modelId)

#decompress and unslice the RAW data
cr2.decode()
#get RAW pixels in a 1D array
rawdata = np.asarray(cr2)
print ('jpeg properties: high=%d, wide=%d, ncomp=%d, bits=%d, hsf=%d, vsf=%d' % (cr2.high, cr2.wide, cr2.ncomp, cr2.bits, cr2.hsf, cr2.vsf))

#into 2D array
raw = np.reshape(rawdata, ( cr2.high, (cr2.wide*cr2.ncomp) ) )
print( 'w=%d h=%d' % ( cr2.wide*cr2.ncomp, cr2.high ) )


if cr2.bits!=15: #RGGB rendering 
  print ("RGGB")
  
  camDngInfo = dng_info[ str(modelId)+'r' ]
  dng_min = camDngInfo[1][0]
  dng_max = camDngInfo[2][0]
   
  if len(sys.argv)>2: #specify borders
    params = sys.argv[2].split(',')
    corners = [int(x) for x in params]
    print ("corners (top,left,bottom,right): ",corners)
    cr2.findBorders( corners )
  else:    #automatically find RGGB borders
    cr2.findBorders( [] )
    
  print ("image borders= [%d-%d] w=%d [%d-%d] h=%d vshift=%d" % (cr2.lborder, cr2.rborder, cr2.width, cr2.tborder, cr2.bborder, cr2.height, cr2.vshift))
  top = cr2.tborder + cr2.vshift #for Digic 3 and 4, APS-C and Full Frame have vshift==1, Powershot don't. Digic>=5 have vshift==0
  bottom = cr2.bborder + cr2.vshift
  print (raw[top:top+4,cr2.lborder:cr2.lborder+4])
  # R G1
  # G2 B
  # red = from top to bottom, each 2 lines AND from left border to right border each two column
  red, g1 = raw[top:bottom:2, cr2.lborder:cr2.rborder:2], raw[top:bottom:2, cr2.lborder+1:cr2.rborder+1:2] 
  g2, blue = raw[top+1:bottom+1:2, cr2.lborder:cr2.rborder:2], raw[top+1:bottom+1:2, cr2.lborder+1:cr2.rborder+1:2]
  colors = [ red, g1, g2, blue ]
  
  #compute WB multiplier from "as shot" ratio
  wb = cr2.getRggbWb()
  print (wb)
  wbmin = float(np.amin(wb))
  wb_mul = [float(x)/wbmin for x in wb]
  print ('wb multipliers=',wb_mul)
  
  mins = [np.amin(x) for x in colors] 
  maxs = [np.amax(x) for x in colors] 
  ev = np.log2( np.amax(maxs)-np.amin(mins) )

  #histogram( colors, ev, ['red','green','green','blue'] )

  counts = [np.shape(x)[0]*np.shape(x)[1] for x in colors] 
  #print counts
  stats( colors, counts ) 

  #linearisation  
  print (dng_min, dng_max)
  dng_min = min(dng_min, np.amin(mins) ) #avoid negative values
  print (dng_min, dng_max)
  tiff_max_val = 65535.0 #unsigned 16bits max value, 0 being the min
  lin_mul = tiff_max_val / float(dng_max-dng_min)
  scale_mul = [x*lin_mul for x in wb_mul]
  print ('scale_mul', scale_mul)

  #black substraction (keep values >=0)
  raw[top:bottom:2, cr2.lborder:cr2.rborder:2] = red - dng_min
  raw[top:bottom:2, cr2.lborder+1:cr2.rborder+1:2] = g1 - dng_min
  raw[top+1:bottom+1:2, cr2.lborder:cr2.rborder:2] = g2 - dng_min 
  raw[top+1:bottom+1:2, cr2.lborder+1:cr2.rborder+1:2] = blue - dng_min

  #color scaling (keep values <= tiff_max_val)
  max_array = np.empty( red.shape, np.uint16 ) 
  max_array.fill( 65535 )
  raw[top:bottom:2, cr2.lborder:cr2.rborder:2] = np.minimum( red * scale_mul[0], max_array)
  raw[top:bottom:2, cr2.lborder+1:cr2.rborder+1:2] = np.minimum( g1 * scale_mul[1], max_array)
  raw[top+1:bottom+1:2, cr2.lborder:cr2.rborder:2] = np.minimum( g2 * scale_mul[2], max_array) 
  raw[top+1:bottom+1:2, cr2.lborder+1:cr2.rborder+1:2] = np.minimum( blue * scale_mul[3], max_array)

  cr2.saveTiff("rggb2_py.tiff")
  
  #interpolation
  img = cr2.createImage()
  pixels = img.getPixels()
  image = np.reshape(pixels, ( img.height, img.width, 4 ) )
  img.interpolate(INTERPOLATION_BILINEAR)  #1=INTERPOLATION_BILINEAR, interpolation of green has been made in G1 column

  img.saveRgbTiff("rgb_py1.tiff")

  #keep only R,G1 and B out of R,G1,G2,B, so we can use matrix multiplication
  rgbimg = np.zeros( ( img.height, img.width, 3 ) )
  rgbimg[:,:,:2] = image[:,:,:2] # R, G
  rgbimg[:,:,2] = image[:,:,3]   # B 
  
  ## color space conversion
  rgb_cam = computeColorMatrix( str(modelId)+'r' )
  if rgb_cam is not None:
    #RGB to sRGB correction
    print (rgbimg.shape)
    rgbimg1 = np.reshape( rgbimg, ( img.height * img.width, 3 ) )
    srgbimg = rgbimg1 * rgb_cam.T
    
    condlist = [ srgbimg < 65536, srgbimg >= 65536 ]
    choicelist = [ srgbimg, 65535 ]
    rgbimg2 = np.reshape( np.asarray( np.select( condlist, choicelist ) ), ( img.height, img.width, 3 ) )
    
    #rgbimg2 = np.reshape( np.asarray(srgbimg), ( img.height, img.width, long(3) ) )
    image[:,:,:2] = rgbimg2[:,:,:2] #R,G
    image[:,:,3] = rgbimg2[:,:,2] #B
    img.saveRgbTiff("rgb_py2.tiff")
  #gamma (if 8bits): something like that
  #http://docs.scipy.org/doc/numpy/reference/generated/numpy.select.html
  #condlist = [r < 0.00304*tiff_max_val, r >= 0.00304*tiff_max_val]
  #choicelist =  [r*12.92, ( 1.055 * (r**(1.0/2.4)) ) - 0.055]
  #np.select(condlist, choicelist )

else:
  print ("YCbCr")
  '''
  mask = np.reshape( cr2.getYuvMask(), (cr2.wide, cr2.high, cr2.ncomp) )
  # using np.tile instead ?
  print(mask)'''
  
  camDngInfo = dng_info[ str(modelId)+'r' ]
  print (camDngInfo)
  dng_min = camDngInfo[1][0]
  dng_max = camDngInfo[2][0]
  dcmin = 0 #dcraw constants
  dcmax = 16383 #2**14 -1
  
  y = np.array( raw[ : , 0: :3], dtype=np.uint16 )
  cb = np.array( raw[ : , 1: :3], dtype=np.int16 ) #signed
  cr = np.array( raw[ : , 2: :3], dtype=np.int16 )
  colors = [y, cb, cr ]

  '''counts = [ np.sum(mask[:,:,x]) for x in range(len(colors)) ] #FIXME
  print (counts)
  stats( colors, counts ) #counts are incorrect, use mask to count only filled pixels
  '''
  wb = cr2.getRggbWb() 
  print ('rggb wb=',wb)
  wbmin = float(np.amin(wb))
  wb_mul = [float(x)/wbmin for x in wb]
  print ('wb multipliers=',wb_mul)
  print ('wb yuv=',cr2.getYuvWb())
  
  #linearisation
  print (dng_min, dng_max)
  tiff_max_val = 65535.0 #unsigned 16bits max value, 0 being the min
  lin_mul = tiff_max_val / float(dcmax-dcmin)
  scale_mul = [x*lin_mul for x in wb_mul]
  print ('scale_mul', scale_mul)
  
  #histogram
  mins = [np.amin(x) for x in colors] 
  maxs = [np.amax(x) for x in colors] 
  mincolor = np.amin( mins )
  maxcolor = np.amax( maxs )
  ev = np.log2( maxcolor-mincolor )
  #histogram( colors, ev, ['Y','Cb','Cr'] )
    
  cr2.yuvInterpolate() #identical to dcraw
  img = cr2.createImageFromYuv() #convert to RGB and apply WB ratio ( with cr2.getYuvWb() values )
  pixels = img.getPixels() #identical to "dcraw922 -w -D -T -4", at end of canon_sraw_load(), compare with "diff_sraw yuv_py1.bin dcraw_yuv_rvb.bin"

  out = open("yuv_py1.bin","wb")
  out.write(pixels)
  out.close()

  img.saveRgbTiff("yuv_py1.tiff")

  
  image = np.reshape(pixels, ( img.height, img.width, 3 ) )
  red, green, blue = image[:,:,0], image[:,:,1], image[:,:,2]
  image[:,:,0] = red * scale_mul[0]
  image[:,:,1] = green * scale_mul[1]
  image[:,:,2] = blue * scale_mul[3]
  img.saveRgbTiff("yuv_py2.tiff")
  
  rgb_cam = computeColorMatrix( str(modelId)+'r' )
  img2 = np.reshape(image, ( img.height* img.width, 3 ) )
  if rgb_cam is not None:
    img3 = img2 * rgb_cam.T
    print (img3.shape)
    image = np.reshape( np.asarray(img3), ( img.height, img.width, 3 ) )
    img.saveRgbTiff("rgb_py3.tiff")


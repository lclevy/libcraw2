from distutils.core import setup, Extension

#python setup.py install

craw2 = Extension( 'craw2', sources=[ 'pyCraw2.c', 'lljpeg.c', 'utils.c', 'craw2.c', 'bits.c', 'tiff.c', 'rgb.c', 'yuv.c' ], 
                   extra_compile_args=[ ], define_macros=[ ('_CRT_SECURE_NO_WARNINGS', None), ('WIN32', None)] )

setup ( name = 'craw2', version = '0.1', description = 'A python extension for libcraw2 library which allows direct handling of .CR2 digital pictures', 
        author = 'Laurent Clevy', url = 'http://lclevy.free.fr/cr2', license = 'LGPL',
        ext_modules = [craw2] )

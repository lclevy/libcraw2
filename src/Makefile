#makefile Linux
#http://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html

CC=gcc
AR=ar
OPTS=
DEFINES=
LIBDIR=/usr/include/python3.6
CFLAGS= -Wall -g -O2 -Wno-unused-result -I$(LIBDIR)

SRCS=craw2tool.c craw2.c utils.c lljpeg.c test_jpeg.c bits.c test_recompress.c tiff.c rgb.c yuv.c pyCraw2.c test_import.c
INCLUDES=craw2.h utils.h lljpeg.h bits.h tiff.h rgb.h yuv.h
FILES=$(INCLUDES) Makefile $(SRCS) test_recomp.sh readme.txt setup.py test_craw2.py test_import.sh dcraw.c \
 vs2008/libcraw2/libcraw2.sln vs2008/libcraw2/libcraw2.vcproj vs2008/craw2tool/craw2tool.vcproj vs2008/pycraw2/pycraw2.vcproj \
 vs2013/craw2tool/craw2tool.vcxproj vs2013/libcraw2/libcraw2.sln vs2013/libcraw2/libcraw2.vcxproj \
 winsdk.bat FILES.txt
WIN64DIR=../vs2008/libcraw2/x64/release
LINUX_BINS=craw2tool craw2tool.exe libcraw2.so libcraw2.a
BINARY_FILES=$(WIN64DIR)/craw2tool.exe $(WIN64DIR)/libcraw2.dll ../dist $(LINUX_BINS)
DEPEND=makedepend
 
OBJS=$(SRCS:%.c=%.o)

all:  craw2tool jpeg libcraw2.so recomp #pycraw2.so import

craw2tool: craw2tool.o libcraw2.a 
	$(CC) -L. -o $@ $< -lcraw2 -lm

diff_sraw: diff_sraw.c
	$(CC) -o $@ $< 

jpeg: test_jpeg.o libcraw2.a 
	$(CC) -L. -o $@ $< -lcraw2 -lm

recomp: test_recompress.o libcraw2.a 
	$(CC) -L. -o $@ $< -lcraw2 -lm

import: test_import.o libcraw2.a 
	$(CC) -L. -o $@ $< -lcraw2 -lm

libcraw2.so: lljpeg.c utils.c craw2.c bits.c tiff.c rgb.c yuv.c
	$(CC) -shared -fPIC -o $@ $^

libcraw2.a: lljpeg.o utils.o craw2.o bits.o tiff.o rgb.o yuv.o
	$(AR) rcs $@ $^ 

#    gcc -DNODEPS -DWIN32 -DMYROW=0 dcraw922.c -o dcraw922 -lws2_32

pycraw2.so: pyCraw2.c libcraw2.so
	$(CC) -c  pyCraw2.c -I. -D MS_WIN64
	$(CC) -shared pyCraw2.o -L/c/Python27/libs -L. -lcraw2 -o $@ -lpython27

clean:
	rm -f *.o *.a *.so craw2tool.exe jpeg.exe recomp.exe jpeg *.bin
	echo >.depend

zip:
	zip -9r libcraw2_`date +%Y%m%d_%H%M%S`.zip $(FILES)

bdist:
	zip -9r craw2_bin_`date +%Y%m%d_%H%M%S`.zip $(BINARY_FILES)
	
depend:
	$(CC) -M $(CFLAGS) $(SRCS) >.depend

include .depend

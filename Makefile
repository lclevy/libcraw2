#makefile Linux
#http://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html

CC=gcc
AR=ar
OPTS=
DEFINES=
LIBDIR=src
CFLAGS= -Wall -g -O2 -Wno-unused-result -I$(LIBDIR)

SRCS=diff_sraw.c grep_tiff.c diff_color.c tohex.c diff_file.c yuv_import.c
INCLUDES=
FILES=$(INCLUDES) Makefile src/*.c src/*.h src/Makefile test_recomp.sh readme.txt src/setup.py test_import.sh \
 vs2008/libcraw2/libcraw2.sln vs2008/libcraw2/libcraw2.vcproj vs2008/craw2tool/craw2tool.vcproj vs2008/pycraw2/pycraw2.vcproj \
 vs2013/craw2tool/craw2tool.vcxproj vs2013/libcraw2/libcraw2.sln vs2013/libcraw2/libcraw2.vcxproj \
 vs2010/craw2tool/craw2tool.vcxproj vs2010/libcraw2/libcraw2.sln vs2010/libcraw2/libcraw2.vcxproj vs2010/pycraw2/pycraw2.vcxproj\
 winsdk.bat FILES.txt diff_color.sh $(SRCS) test_tiff.sh cr2_database.sh samples_list_rggb.txt samples_list_yuv.txt user_manual.md \
 dng_info.sh databases.md noregression.sh compile.md pycraw2_demo.py libcraw2_dev.md dcraw927.diff
WIN64DIR=../vs2008/libcraw2/x64/release
LINUX_BINS=craw2tool craw2tool.exe libcraw2.so libcraw2.a
BINARY_FILES=$(WIN64DIR)/craw2tool.exe $(WIN64DIR)/libcraw2.dll ../dist $(LINUX_BINS)
DEPEND=makedepend
 
OBJS=$(SRCS:%.c=%.o)

all: diff_color diff_sraw grep_tiff yuv_import src/craw2tool.exe

src/craw2tool.exe: 
	cd src; make; cd ..

diff_sraw: diff_sraw.c 
	$(CC) -o $@ $< 

diff_file: diff_file.c 
	$(CC) -o $@ $< 

diff_color: diff_color.c  
	$(CC) -o $@ $<

grep_tiff: grep_tiff.o src/tiff.o src/craw2.o src/lljpeg.o src/bits.o src/utils.o src/yuv.o src/rgb.o
	$(CC) $(CFLAGS) -o $@ $?

yuv_import: yuv_import.o src/tiff.o src/craw2.o src/lljpeg.o src/bits.o src/utils.o src/yuv.o src/rgb.o -lm
	$(CC) $(CFLAGS) -o $@ $?
  
clean:
	rm -f *.o *.a *.so grep_tiff.exe jpeg.exe jpeg *.bin
	cd src; make clean; cd ..
	echo >.depend

zip:
	zip -9r libcraw2_`date +%Y%m%d_%H%M%S`.zip $(FILES)

bdist:
	zip -9r craw2_bin_`date +%Y%m%d_%H%M%S`.zip $(BINARY_FILES)
	
depend:
	$(CC) -M $(CFLAGS) $(SRCS) >.depend

include .depend

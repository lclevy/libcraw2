Tested with Python 3.6.7, gcc 7.3.0 and Ubuntu 18.04


* requirements for Ubuntu 18.04

- maybe you would like to use shared folders:

$ sudo mount -t fuse.vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other

- required packages:

install gcc 7 : sudo apt install gcc
install make : sudo apt install make
install python 3 : sudo apt install python3; sudo apt install python3-dev

update Makefile with python include DIR = /usr/include/python3.6

- build craw2tool

cd src
make clean
make

- build pyCraw2

cd src
sudo python3 setup.py install

- test PyCraw2

install pip : sudo apt install python3-pip
install numpy : pip3 install numpy

* successfull memory tests

valgrind --leak-check=full --show-leak-kinds=all src/craw2tool -e yuv16i pics/IMG_0590_mraw.CR2 
valgrind --leak-check=full --show-leak-kinds=all src/craw2tool -e rggb pics/5d2.cr2 
valgrind --leak-check=full --show-leak-kinds=all src/craw2tool -e yuv16p pics/IMG_0590_mraw.CR2 









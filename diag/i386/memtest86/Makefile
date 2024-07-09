# Makefile for MemTest-86
#
# Author:		Chris Brady
# Created:		January 1, 1996
# Enhancements - V1.1:	May 28, 1996

#
# Select Linux release (Must be set for loader compatibility. Only Releases
# 1.2 and 1.3 are currently supported)
#
#REL_1.2=1
REL_1.3=1

#
# Path for the floppy disk device
#
FDISK=/dev/fd0

#
# Comment out the following to disable testing without cache
#
CACHE=-DCACHE

#
# Comment out the following to disable testing with long refresh timing
#
REFRESH=-DREFRESH

#
# gcc compiler options, these settings should suffice
#
CCFLAGS=-m486 -O2

all: setup bootsect head build
ifdef REL_1.3

	objdump -k -q -o 0x0100 head >head.out
	./build bootsect setup head.out >image
else
	./build bootsect setup head >image
endif

mtest.o: mtest.c mtest.h
	cc -c $(CCFLAGS) $(REFRESH) $(CACHE) mtest.c

# Warning - Read the following carefully if you modify the code in any way!
#
# The "-Ttext" option for ld sets the origin for the text segment. This
# must match the adrs where the code will execute (TESTADR in mtest.h). 
# However, in Linux 1.2 ld introduces an offset of 0x20 so the
# specified address must be set to TESTADR - 0x20. Ugly!!
#
# The "-Tdata" option for ld sets the origin for the data segment. This is
# is set by hand and will need to be adjusted if the code grows much at all.
# The current settings leave only a small amount of room for growth. Look at
# the loader map to check this address.  In addition you may need to adjust
# the starting test address (START_ADR in mtest.h) to the end address of the
# test + ~0x20.  Again consult the loader map.
#
head: head.o mtest.o
ifdef REL_1.3
	ld -m elf_i386 -o $@ -e do_test -Ttext 0x0100 -Tdata 0x0cc0 -Map mapfile head.o mtest.o
else
	ld -o $@  -qmagic -Map mapfile -Ttext 0x00e0 -Tdata 0x0cc0 head.o mtest.o
endif

head.o: head.s
	as -o $@ $<

head.s: head.S mtest.h
	cc -E -traditional $< -o $@

setup: setup.o
	ld86 -0 -s -o $@ $<

setup.o: setup.s
	as86 -a -0 -o $@ $<

setup.s: setup.S mtest.h
	cc -E -traditional $< -o $@

bootsect: bootsect.o
	ld86 -0 -s -o $@ $<

bootsect.o: bootsect.s
	as86 -a -0 -o $@ $<

bootsect.s: bootsect.S mtest.h
	cc -E -traditional $< -o $@

build:	build.c
ifdef REL_1.3
	cc -DELF -o build build.c
else
	cc -o build build.c
endif

clean:
	rm -f *.o *.s build image bootsect setup head head.out mapfile

install: all
	dd <image >$(FDISK) bs=8192

install-bin:
	dd <image.bin >$(FDISK) bs=8192


#	BSDI i386bsdi.sh,v 1.1 2003/01/14 16:48:47 torek Exp

SCRIPT_NAME=aout
OUTPUT_FORMAT="a.out-i386-bsd"
TARGET_PAGE_SIZE=0x1000
TEXT_START_ADDR=0x1020
NONPAGED_TEXT_START_ADDR=0
ARCH=i386
TEMPLATE_NAME=generic

# ??? I am not sure if I am supposed to override it this way,
# but we must force this to appear to be a "default" emulation.
EMULATION_LIBPATH=i386bsdi

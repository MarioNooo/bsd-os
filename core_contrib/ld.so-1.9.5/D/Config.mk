ARCH = i386
#ARCH = m68k
#ARCH = sparc
#DEBUG = true

ifeq ($(ARCH),sparc)
AOUT_SUPPORT = false
else
AOUT_SUPPORT = true
endif

LDSO_ADDR = 62f00020
LDSO_ENTRY = "0x$(LDSO_ADDR)"

# Do NOT use -fomit-frame-pointer -- It won't work!
CFLAGS	= -Wall -O4 -g -DVERSION=\"$(VERSION)\" -D_GNU_SOURCE
ifeq ($(DEBUG),true)
CFLAGS  += -DDEBUG
endif

CC = gcc
AS = as
LD = ld
RANLIB = ranlib

ifeq ($(ARCH),i386)
LIBC5_CC = gcc -b i486-linuxlibc1
else
LIBC5_CC = gcc -b $(ARCH)-linuxlibc1
endif

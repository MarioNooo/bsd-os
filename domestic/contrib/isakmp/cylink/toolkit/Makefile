CC	= gcc
CFLAGS	= -g -DCRYPTO_TOOLKIT
DFLAGS  = -g -DCRYPTO_TOOLKIT
LD	= $(CC) 
#LDFLGAS = -z muldefs
LDFLAGS = 
AS	= as
OBJS	= main.o test.o
BNDIR   = ../bignum
PROG	= crypto
LIB	= libcrypto.a

#
# NetBSD uncomment out following
#CFLAGS+=-DNetBSD

XINCLUDES = -I$(BNDIR)
#
# For Solaris SPARC ASM uncomment the following two lines...
#
#DEFINES-sparc	= -DB_ENDIAN -DASM_FUNCTION -DORD_32
#LIBOBJS-sparc	= bits.o des.o dh.o dnh.o dss.o math.o prime.o\
#rand.o sha.o fipssha.o swap.o des6.o

#
# For Solaris SPARC C uncomment the following two lines...
#
#DEFINES-sparc	= -DB_ENDIAN -DC_FUNCTION -DORD_32
#LIBOBJS-sparc	= cencrypt.o cencrint.o c_asm.o

#
# Solaris I386 C defines.
#
#DEFINES-i386	= -DL_ENDIAN -DC_FUNCTION -DORD_16
#LIBOBJS-i386	= cencrypt.o cencrint.o c_asm.o
DEFINES		= -DL_ENDIAN -DC_FUNCTION -DORD_32
LIBOBJS		= bits.o dh.o dnh.o des.o dss.o math.o prime.o\
rand.o sha.o fipssha.o swap.o keyshare.o
#
#DEFINES = $(DEFINES$(TARGET_ARCH))
#LIBOBJS = $(LIBOBJS$(TARGET_ARCH))

all: $(LIB)

.c.o:
	$(CC) -c $(CFLAGS) $(DEFINES) ${XINCLUDES} $<

.s.o:
	$(AS) -o $@ $<

$(PROG): $(LIB) $(LIBOBJS) $(OBJS)
	$(LD) -o $(PROG) $(OBJS) $(LIB) ../bignum/libbn.a -lm

$(LIB): $(LIBOBJS)
	ar -cq $(LIB) $(LIBOBJS)
	ranlib $(LIB)


debug:
	@make "CC=cc $(DFLAGS) -DDEBUG" $(PROG)

insight:
	@make "CC=insight" $(PROG)

insight-debug:
	@make "CC=insight -DDEBUG" $(PROG)

#diffs:	cleandiffs
#	/usr/genie/dan/work/bin/diffem CRYPTO

cleandiffs:
	rm -f diffs

clean:
	rm -f *.o core *.out *.log *.map $(PROG) $(LIB) diffs

install:


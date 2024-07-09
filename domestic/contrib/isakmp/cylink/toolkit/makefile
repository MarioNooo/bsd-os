LIB=	crypto
SRCS=	bits.c dh.c dnh.c des.c dss.c math.c prime.c
SRCS+=	rand.c sha.c fipssha.c swap.c keyshare.c
CFLAGS+= -g -DCRYPTO_TOOLKIT
.if ${MACHINE} == "i386"
CFLAGS+= -DL_ENDIAN -DC_FUNCTION -DORD_32
.elif ${MACHINE} == "sparc" || ${MACHINE} == "sparc_v9"
CFLAGS+= -DB_ENDIAN -DASM_FUNCTION -DORD_32
SRCS+=	des6.s
.endif

NOPROFILE=noprofile

DFLAGS  = -g -DCRYPTO_TOOLKIT
#LDFLGAS = -z muldefs
LDFLAGS = 
CRYPTO_OBJS = main.o test.o
BNDIR   = ${.CURDIR}/../bignum
PROG	= crypto

CFLAGS += -I$(BNDIR) -I$(BNDIR)/obj

$(PROG): lib$(LIB).a $(CRYPTO_OBJS)
	$(LD) -o $(PROG) $(CRYPTO_OBJS) lib$(LIB).a ../bignum/obj/libbn.a -lm

debug:
	@make "CC=cc $(DFLAGS) -DDEBUG" $(PROG)

install:

.include <bsd.lib.mk>

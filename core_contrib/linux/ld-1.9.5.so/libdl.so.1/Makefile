#	BSDI Makefile,v 1.3 1999/10/09 04:00:51 donn Exp

.if ${MACHINE} == "sparc_v9"
MCH=	sparc
.else
MCH=	${MACHINE}
.endif

PROG=	libdl.so.1

SRCS=   dlib.c

LD_DIR=	${.CURDIR}/../../../ld.so-1.9.5
DLINK=	${LD_DIR}/D/d-link
INCLUDE=${LD_DIR}/include

CFLAGS+=-g -fPIC -I- -I${INCLUDE} -I${DLINK}/libdl -I${DLINK}/${MCH} \
	-I${DLINK} -D__PIC__ -DNO_UNDERSCORE -DUSE_CACHE \
	-DUSE_LCALL -DVERBOSE_DLINKER
LDCC=	${LD} -shared -h libdl.so.1
STRIP=
NOMAN=	noman
BINDIR=	/linux/usr/i486-linux-libc5/lib

.PATH:	${DLINK}/libdl

.include <bsd.prog.mk>

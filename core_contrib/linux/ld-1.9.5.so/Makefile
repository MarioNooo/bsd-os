#	BSDI Makefile,v 1.2 1999/01/08 16:24:29 prb Exp

.if ${MACHINE} == "sparc_v9"
MCH=	sparc
.else
MCH=	${MACHINE}
.endif

PROG=	ld-1.9.5.so
SUBDIR=	libdl.so.1

SRCS=   boot1.c hash.c readelflib1.c vsprintf.c

LD_DIR=	${.CURDIR}/../../ld.so-1.9.5
DLINK=	${LD_DIR}/D/d-link
INCLUDE=${LD_DIR}/include

CFLAGS+=-g -fPIC -I- -I${.CURDIR}/dummy -I${INCLUDE} -I${DLINK}/${MCH} \
	-I${DLINK} -D__PIC__ -DMAP_ANONYMOUS=MAP_ANON \
	-DNO_UNDERSCORE -DUSE_CACHE -DUSE_PRELOAD -DVERBOSE_DLINKER \
	-DUSE_LCALL -DUSE_WEAK -DVERSION=\"1.9.5\"
LDCC=	ld -e _dl_boot -shared
NOMAN=	noman
BINDIR=	/linux/lib
LINKS=	${BINDIR}/ld-1.9.5.so ${BINDIR}/ld-linux.so.1 \
	${BINDIR}/ld-1.9.5.so ${BINDIR}/ld-linux.so.1.9.5
STRIP=

.PATH:	${DLINK}/${MCH} ${DLINK}

.include <${LD_DIR}/d-link/Makefile.${MCH}>
.include <bsd.prog.mk>

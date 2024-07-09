#	BSDI Makefile,v 1.5 1998/09/09 05:20:33 torek Exp

.if ${MACHINE} == "sparc_v9"
MCH=	sparc
.else
MCH=	${MACHINE}
.endif

PROG=	ld-bsdi.so

SRCS=   boot1.c hash.c readelflib1.c vsprintf.c

DLINK=	${.CURDIR}/../D/d-link

CFLAGS+=-fPIC -I- -I${.CURDIR}/../include -I${DLINK}/${MCH} -I${DLINK} \
	-D__PIC__ -DIBCS_COMPATIBLE -DMAP_ANONYMOUS=MAP_ANON \
	-DNO_UNDERSCORE -DUSE_CACHE -DVERBOSE_DLINKER -DVERSION=\"1.9.5\"
LDCC=	ld -e _dl_boot -shared
MAN8=	ld.so.0
MLINKS=	ld.so.8 ld-bsdi.so.8
BINDIR=	/shlib
STRIP=

.PATH:	${DLINK}/${MCH} ${DLINK}

.include <Makefile.${MCH}>
.include <bsd.prog.mk>

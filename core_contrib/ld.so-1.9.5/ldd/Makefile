#	BSDI Makefile,v 1.1 1998/05/06 19:15:28 donn Exp

PROG=	ldd
SUBDIR=	lddstub

CFLAGS+=-I- -I${.CURDIR}/../include -I${.CURDIR}/../D/util \
	-DIBCS_COMPATIBLE -DMAP_ANONYMOUS=MAP_ANON \
	-DNO_UNDERSCORE -DUSE_CACHE -DVERBOSE_DLINKER -DVERSION=\"1.9.5\"
LDCC=	cc -static

.PATH:	${.CURDIR}/../D/util

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.prog.mk>

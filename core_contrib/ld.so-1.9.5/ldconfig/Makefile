#	BSDI Makefile,v 1.4 1998/02/11 23:24:56 donn Exp

PROG=	ldconfig

SRCS=   ldconfig.c readelf.c

CFLAGS+=-I- -I${.CURDIR}/../include -I${.CURDIR}/../D/util \
	-DIBCS_COMPATIBLE -DMAP_ANONYMOUS=MAP_ANON \
	-DNO_UNDERSCORE -DUSE_CACHE -DVERBOSE_DLINKER -DVERSION=\"1.9.5\"
LDCC=	cc -static
BINDIR=	/sbin
MAN8=	ldconfig.0

.PATH:	${.CURDIR}/../D/util

.include <bsd.prog.mk>

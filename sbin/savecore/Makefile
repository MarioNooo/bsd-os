#	BSDI	Makefile,v 2.5 1998/09/14 20:33:37 torek Exp
#	@(#)Makefile	8.2 (Berkeley) 4/17/94

PROG=	savecore
SRCS=	savecore.c zopen.c
CFLAGS+=-I/sys
MAN8=	savecore.0
.PATH:	${.CURDIR}/../../usr.bin/compress

.include <bsd.prog.mk>

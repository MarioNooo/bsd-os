#	BSDI Makefile,v 1.3 1995/10/17 20:10:58 bostic Exp

PROG=	n2setup
SRCS=	gram.y scan.l n2setup.c
CFLAGS+=-I/sys -I${.CURDIR} -I.
LFLAGS+=-i
MAN8=	n2setup.0
CLEANFILES=gram.c scan.c y.tab.h

.include <bsd.prog.mk>

.depend: gram.c scan.c

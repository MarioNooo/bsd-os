#	BSDI Makefile,v 1.2 1999/07/29 16:14:44 cp Exp

PROG=	wanic500
SRCS=	gram.y scan.l wanic500.c
CFLAGS+=-I/sys -I${.CURDIR} -I.
LFLAGS+=-i
MAN8=	wanic500.0
CLEANFILES=gram.c scan.c y.tab.h

.include <bsd.prog.mk>

.depend: gram.c scan.c

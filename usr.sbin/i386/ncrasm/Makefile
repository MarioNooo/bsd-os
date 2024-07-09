#	@(#)Makefile	5.1 (Berkeley) 1/12/93

PROG=	ncrasm
SRCS=	gram.y scan.l main.c symbol.c script.c
CFLAGS+=-I${.CURDIR} -I. -I/sys
YFLAGS+=-v 
CLEANFILES=gram.c scan.c y.tab.h y.output

MAN8=	ncrasm.0

.include <bsd.prog.mk>

.depend: gram.c scan.c

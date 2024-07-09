#	@(#)Makefile	5.1 (Berkeley) 1/12/93

PROG=	aicasm
SRCS=	gram.c scan.c main.c symbol.c sequencer.c
#CFLAGS= -g -D"YYDEBUG=1"
CFLAGS+= -g
CFLAGS+=-I${.CURDIR} -I. -I${.CURDIR}/../../../sys
YFLAGS+=-v 
CLEANFILES=gram.c scan.c y.tab.h y.output

MAN8=	aicasm.0

.include <bsd.prog.mk>

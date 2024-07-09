#	BSDI Makefile,v 1.5 2003/06/06 00:09:10 polk Exp

PROG=	ipfwcmp
SRCS=	yacc.y lex.l ipfwcmp.c
MAN8=	ipfwcmp.0
YFLAGS+=-t -v
CFLAGS+=-I. -I${.CURDIR}
CLEANFILES+=	y.output y.tab.h
CFLAGS+=-g -Wall

lex.o::	yacc.y

.include <bsd.prog.mk>


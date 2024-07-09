#	BSDI Makefile,v 1.7 2003/06/06 00:08:00 polk Exp

PROG=	ipfwasm
MAN8=	ipfwasm.0

YFLAGS+=-t -v
CFLAGS+=-I. -I${.CURDIR} -Wall
CFLAGS+=-I${.CURDIR}/../ipfw

SRCS=	ipfwasm.c yacc.y lex.l
CLEANFILES+=	y.output y.tab.h

.include <bsd.prog.mk>

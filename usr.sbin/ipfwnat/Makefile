#	BSDI Makefile,v 1.6 2003/06/06 00:09:57 polk Exp

PROG=	ipfwnat

.PATH:  ${.CURDIR}/../ipfw
SRCS=	ipfwnat.c yacc.y lex.l ipfw_natdump.c
CFLAGS+=-I. -I${.CURDIR}
YFLAGS+=-t -v
CLEANFILES+=	y.output y.tab.h
MAN8=	ipfwnat.0

.include <bsd.prog.mk>

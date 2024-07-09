#	BSDI Makefile,v 2.3 1995/10/17 20:10:47 bostic Exp

PROG=	ioport
CFLAGS+=-I/sys
MAN8=	ioport.0
DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>

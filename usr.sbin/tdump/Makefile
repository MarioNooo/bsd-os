#	BSDI Makefile,v 1.1 2000/12/17 16:24:23 torek Exp

PROG=	tdump
CFLAGS+=-I/sys -Wall
MAN8=	tdump.0
DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>

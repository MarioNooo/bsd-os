#	BSDI	Makefile,v 1.2 1996/06/12 15:24:49 bostic Exp

PROG=	splice
MAN8=	splice.0
CFLAGS+= -I/sys

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

#	BSDI Makefile,v 1.4 2003/08/21 00:05:04 polk Exp

BINDIR= /usr/bin

PROG=	x99token
SRCS=	x99token.c
CFLAGS+=-DKRBDES
LDADD+=	-lcrypto

.include <bsd.prog.mk>

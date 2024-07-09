# BSDI	Makefile,v 2.12 2002/02/27 23:34:59 polk Exp

PROG=	login
SRCS=	login.c isppp.c license.c
.PATH:	${.CURDIR}/../../libexec/getty
DPADD=	${LIBUTIL}
LDADD=	-lutil -ldialer

BINOWN=	root
BINMODE=500
MAN8=	login.0

.include <bsd.prog.mk>

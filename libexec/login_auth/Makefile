#	BSDI Makefile,v 1.3 2003/08/21 22:29:27 polk Exp

PROG=	login_auth
SRCS=	login_auth.c auth_subr.c auth_comm.c des.c
MAN8=	login_auth.0
.PATH:	${.CURDIR}/../authsrv
CFLAGS+=-I${.CURDIR}/../authsrv
BINMODE=4555
BINOWN=	root

LDADD+=-lutil -lcrypto
CFLAGS+=-Wall

.include <bsd.prog.mk>

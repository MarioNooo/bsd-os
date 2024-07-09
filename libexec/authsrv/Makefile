#	BSDI Makefile,v 1.2 2003/08/21 22:29:27 polk Exp

PROG=	authsrv
SRCS=	pty.c authsrv.c auth_subr.c auth_comm.c des.c
MAN8=	authsrv.0
MAN4=	auth.0

LDADD+=-lutil -lcrypto
CFLAGS+=-Wall

.include <bsd.prog.mk>

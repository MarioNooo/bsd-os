#	BSDI	Makefile,v 1.8 2002/02/27 23:34:58 polk Exp

PROG=	login_passwd
DPADD=	${LIBUTIL} /usr/lib/libauth.a
LDADD=	-lutil
CFLAGS+=-DPASSWD -Wall
MAN8=	login_passwd.0

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>

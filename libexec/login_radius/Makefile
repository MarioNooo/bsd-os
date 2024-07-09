#	BSDI Makefile,v 1.3 1998/04/14 00:39:02 prb Exp

PROG=	login_radius
SRCS=	login_radius.c raddauth.c md5.c
BINDIR?=/usr/libexec
LDADD+=-lutil
BINOWN= root
BINMODE=4555
MAN8=	login_radius.0

.include <bsd.prog.mk>

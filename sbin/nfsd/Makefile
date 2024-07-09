#	BSDI Makefile,v 2.2 1996/09/19 14:23:09 bostic Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	nfsd
CFLAGS+=-DNFS
MAN8=	nfsd.0

DPADD=	${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>

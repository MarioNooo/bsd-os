#	BSDI Makefile,v 2.3 2003/09/04 18:02:23 dab Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	inetd
SRCS=	inetd.c ipsec.c
MAN8=	inetd.0
MLINKS=	inetd.8 inetd.5
DPADD=	${LIBUTIL} ${LIBIPSEC}
LDADD=	-lutil -lipsec
CFLAGS+= -DIPSEC -DINET6

.include <bsd.prog.mk>

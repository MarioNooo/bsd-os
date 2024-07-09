#	BSDI Makefile,v 2.2 1996/03/29 03:05:54 prb Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	getty
SRCS=	main.c init.c subr.c ttydefaults.c bidir.c uucplock.c isppp.c
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN5=	gettytab.0 ttys.0
MAN8=	getty.0

.PATH:	${.CURDIR}/../../usr.bin/tip

.include <bsd.prog.mk>

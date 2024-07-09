#	BSDI Makefile,v 2.2 2002/02/27 23:34:59 polk Exp
#	@(#)Makefile	8.2 (Berkeley) 4/2/94

PROG=	chpass
SRCS=	chpass.c edit.c field.c table.c util.c
BINOWN=	root
BINMODE=4555
LINKS=	${BINDIR}/chpass ${BINDIR}/chfn ${BINDIR}/chpass ${BINDIR}/chsh
MLINKS=	chpass.1 chfn.1 chpass.1 chsh.1
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

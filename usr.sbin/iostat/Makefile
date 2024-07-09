#	BSDI Makefile,v 2.2 1996/06/12 15:12:10 bostic Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	iostat
MAN8=	iostat.0
CFLAGS+=-DBACKWARD_COMPATIBILITY
BINGRP=	kmem
BINMODE=2555
DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>

#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	pstat
MAN8=	pstat.0
SRCS=	cd9660.c pstat.c
BINGRP=	kmem
BINMODE=2555
DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>

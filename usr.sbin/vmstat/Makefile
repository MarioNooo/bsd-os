#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	vmstat
SRCS=	names.c vmstat.c
CFLAGS+=-DBACKWARD_COMPATIBILITY
MAN8=	vmstat.0
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

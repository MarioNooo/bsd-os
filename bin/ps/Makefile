#	@(#)Makefile	8.1 (Berkeley) 6/2/93

PROG?=	ps
MAN1?=	ps.0
SRCS=	fmt.c keyword.c nlist.c print.c ps.c
DPADD=	${LIBMATH} ${LIBKVM}
LDADD=	-lm -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

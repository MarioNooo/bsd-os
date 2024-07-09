SRCDIR=	${.CURDIR}/../../kame/mld6query

PROG=	mld6query
SRCS=	mld6.c

MAN8=	mld6query.0

BINOWN=	root
BINGRP=	bin

CFLAGS+= -DINET6 -DIPSEC -DUSE_RFC2292BIS

.PATH:	${SRCDIR}

.include <bsd.prog.mk>

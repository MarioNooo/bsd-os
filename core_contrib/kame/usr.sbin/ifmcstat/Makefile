PROG=	ifmcstat
MAN8=	ifmcstat.0

BINGRP=	kmem
BINMODE=2555
LDADD=	-lkvm
DPADD=	${LIBKVM}

CFLAGS+=	-DINET6

.include <bsd.prog.mk>

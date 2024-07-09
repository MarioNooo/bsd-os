
TCPDUMP=${.CURDIR}/../../../tcpdump/tcpdump-3.4a6
.PATH:	${TCPDUMP}

PROG=	ndp
SRCS=	ndp.c gmt2local.c
MAN8=	ndp.0
#MAN4=	ndp.0

CFLAGS+=-DINET6
CFLAGS+= -I${.CURDIR} -I${TCPDUMP}

.include <bsd.prog.mk>

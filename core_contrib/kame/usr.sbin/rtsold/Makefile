PROG=	rtsold
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CFLAGS+=-DINET6 -DHAVE_GETIFADDRS

MAN8=	rtsold.0
MLINKS=	rtsold.8 rtsol.8

.include <bsd.prog.mk>

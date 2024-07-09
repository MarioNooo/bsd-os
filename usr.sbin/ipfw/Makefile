#	BSDI	Makefile,v 1.5 2000/01/24 21:52:44 prb Exp

PROG=	ipfw
MAN8=	ipfw.0
SRCS=	ipfw.c
SRCS+=	ipfw_circutdump.c ipfw_natdump.c ipfw_throttledump.c ipfw_flowdump.c \
	ipfw_cachedump.c

CFLAGS+=-Wall -g

.include <bsd.prog.mk>

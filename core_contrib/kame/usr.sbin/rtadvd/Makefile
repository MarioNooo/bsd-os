
PROG=	rtadvd
SRCS=	rtadvd.c rrenum.c advcap.c if.c config.c timer.c dump.c

CFLAGS+=-DINET6 -DMIP6
LDADD+=	-lcompat
DPADD+=	${LIBCOMPAT}

MAN5=	rtadvd.conf.0
MAN8=	rtadvd.0

.include <bsd.prog.mk>

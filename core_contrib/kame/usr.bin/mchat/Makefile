
PROG=	mchat
SRCS=	mchat.c screen.c history.c

CFLAGS+=-DINET6
CFLAGS+=-DIPSEC
CFLAGS+=-Dss_family=__ss_family -Dss_len=__ss_len
LDADD+=	-lipsec
LDADD+=-lcurses -ltermcap
DPADD+=${LIBCURSES} ${LIBTERMCAP}

MAN1=	mchat.0

.include <bsd.prog.mk>

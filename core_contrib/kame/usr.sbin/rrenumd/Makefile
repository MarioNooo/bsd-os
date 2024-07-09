PROG=	rrenumd
SRCS=	rrenumd.c  parser.y lexer.l
#YFLAGS+=	-d

CFLAGS+=-DINET6 -I${.CURDIR} -I.
#CFLAGS+=-DIPSEC
#LDADD+=	-lipsec
LDADD+=	-lcompat
DPADD+=	${LIBCOMPAT}
LDADD+= -ll -ly
DPADD+=	${LIBL} ${LIBY}

MAN5=	rrenumd.conf.0
MAN8=	rrenumd.0

.if defined(YACCDEBUG)
CFLAGS+= -DYYDEBUG
YFLAGS+= -t -v
.endif

.include <bsd.prog.mk>

CLEANFILES+=	y.tab.h

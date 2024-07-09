
PROG=	bgpd
SRCS=	main.c if.c rt_table.c task.c parse.c in6.c router.c \
	bgp.c bgp_input.c bgp_output.c bgp_util.c aspath.c \
	ripng.c ospf.c dump.c cftoken.l cfparse.y
CLEANFILES+=	lex.yy.c y.tab.h y.tab.c
YFLAGS+=-d

MAN5=	bgpd.conf.0
MAN8=	bgpd.0

CFLAGS+=-I${.OBJDIR} -I${.CURDIR}
CFLAGS+=-Wall
CFLAGS+=-DHAVE_STDARG_H -DADVANCEDAPI -DINET6 -DUSE_RFC2292BIS -DYY_NO_UNPUT
CFLAGS+=-DHAVE_GETIFADDRS

LDADD+=	-lcompat
DPADD+= ${LIBCOMPAT}
LDADD+=-ly -ll
DPADD+=	${LIBY} ${LIBL}

cftoken.o: y.tab.h
y.tab.h: cfparse.o

.include <bsd.prog.mk>

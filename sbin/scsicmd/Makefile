#	BSDI Makefile,v 2.5 2001/06/26 20:13:15 polk Exp

PROG=	scsicmd
SRCS=	main.c support.c parse.y scan.l
MAN8=	scsicmd.0

scan.o:	parse.c
.depend: parse.c scan.c

CLEANFILES+=	parse.c scan.c y.tab.h

CFLAGS+=	-I/sys -I${.CURDIR} -I.
YFLAGS=		-d
LDADD+=		-lutil -ll
DPADD+=		${LIBUTIL} ${LIBL}

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/scsicmdtab \
		${DESTDIR}/usr/share/misc/scsicmdtab
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${.CURDIR}/wce \
		${DESTDIR}/usr/sbin/wce

.include <bsd.prog.mk>

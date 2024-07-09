#	BSDI Makefile,v 1.1 1998/08/28 12:09:42 torek Exp

LINKS=	${BINDIR}/w ${BINDIR}/uptime

obj objdir clean cleandir:

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/../../bin/ps.sparc/ps.sh ${DESTDIR}${BINDIR}/w

.include <bsd.prog.mk>

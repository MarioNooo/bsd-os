#	BSDI Makefile,v 1.1 1998/08/28 10:59:22 torek Exp

obj objdir clean cleandir:

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/ps.sh ${DESTDIR}${BINDIR}/ps

.include <bsd.prog.mk>

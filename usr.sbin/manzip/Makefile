# BSDI Makefile,v 1.2 1995/10/27 14:32:34 bostic Exp

MAN8=	manzip.0

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/manzip.pl ${DESTDIR}${BINDIR}/manzip

.include <bsd.prog.mk>

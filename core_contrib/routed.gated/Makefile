# BSDI Makefile,v 1.1 2002/10/18 21:29:54 polk Exp

BINDIR=	/usr/sbin
MAN8=	routed.0

routed clean depend lint tags:

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/routed.pl ${DESTDIR}/${BINDIR}/routed
	ln -fs ../usr/sbin/routed ${DESTDIR}/sbin/routed

cleandir:
	rm -f routed.0

.include <bsd.prog.mk>

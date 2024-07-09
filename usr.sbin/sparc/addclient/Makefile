#	BSDI Makefile,v 1.1 1997/04/30 19:19:55 torek Exp

MAN8=	addclient.0

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 555 \
	    ${.CURDIR}/addclient.sh ${DESTDIR}${BINDIR}/addclient

.include <bsd.prog.mk>

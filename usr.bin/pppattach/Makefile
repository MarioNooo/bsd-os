#	BSDI Makefile,v 1.1 1997/01/23 06:52:56 prb Exp
MAN8=	pppattach.0

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/pppattach ${DESTDIR}/usr/bin/pppattach

.include <bsd.prog.mk>

#	BSDI Makefile,v 1.1 1997/04/17 17:04:03 polk Exp

MAN8=   modlog.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	  ${.CURDIR}/modlog ${DESTDIR}${BINDIR}/

.include <bsd.prog.mk>

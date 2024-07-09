#	BSDI	Makefile,v 1.1 1996/11/12 21:28:35 sanders Exp

MAN1=   rotate.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/rotate.sh ${DESTDIR}${BINDIR}/rotate

.include <bsd.prog.mk>

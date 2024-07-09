#	BSDI Makefile,v 1.1 1995/10/24 20:55:50 cp Exp

MAN8=	ioprod.0

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/ioprod.pl ${DESTDIR}${BINDIR}/ioprod

.include "../../Makefile.inc"
.include <bsd.prog.mk>

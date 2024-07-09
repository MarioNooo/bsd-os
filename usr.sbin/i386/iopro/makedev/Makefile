#	BSDI Makefile,v 1.1 1995/10/24 21:12:39 cp Exp

BINDIR?= /dev

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/MAKEDEV.iopro.sh ${DESTDIR}${BINDIR}/MAKEDEV.iopro

.include "../../Makefile.inc"
.include <bsd.prog.mk>

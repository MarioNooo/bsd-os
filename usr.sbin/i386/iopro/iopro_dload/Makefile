#	$BSDI Makefile,v 1.1 1995/10/24 21:05:20 cp Exp

PROG=		iopro_dload
BINDIR?=	/usr/libexec
MAN8=		iopro_dload.0
FILES=		iopro.dl
CFLAGS+=-I/sys

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${.CURDIR}/iopro.dl ${DESTDIR}/usr/libdata

.include "../../Makefile.inc"
.include <bsd.prog.mk>

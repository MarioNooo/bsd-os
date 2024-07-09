# BSDI Makefile,v 1.3 1995/11/04 17:50:01 bostic Exp

MAN1=	s2p.0

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/s2p.pl ${DESTDIR}/${BINDIR}/s2p

.include "../../Makefile.inc"
.include <bsd.prog.mk>

#	BSDI Makefile,v 2.4 1996/08/21 21:19:35 bostic Exp

PROG=   vgafont
FONTDIR=${DESTDIR}/usr/share/vgafont
BINOWN=root
BINMODE=4555

afterinstall:
	-[ -d ${FONTDIR} ] || mkdir ${FONTDIR}
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${.CURDIR}/fonts/[a-z]* ${FONTDIR}

.include <bsd.prog.mk>

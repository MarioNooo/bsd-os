#	BSDI Makefile,v 2.1 1995/02/03 12:38:35 polk Exp

MAN1=	nroff.0

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/nroff.sh ${DESTDIR}${BINDIR}/nroff

.include "../Makefile.defs"
.include "../Makefile.common"
.include <bsd.prog.mk>

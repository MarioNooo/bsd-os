#	BSDI Makefile,v 2.1 1995/02/03 12:25:33 polk Exp

MAN1=	afmtodit.0

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/afmtodit.pl ${DESTDIR}${BINDIR}/afmtodit

.include "../Makefile.defs"
.include "../Makefile.common"
.include <bsd.prog.mk>

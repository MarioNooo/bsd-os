#	BSDI Makefile,v 2.1 1995/02/03 12:32:52 polk Exp

MAN1=	grog.0

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/grog.pl ${DESTDIR}${BINDIR}/grog

.include "../Makefile.defs"
.include "../Makefile.common"
.include <bsd.prog.mk>

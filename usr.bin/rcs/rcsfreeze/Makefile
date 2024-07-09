#	BSDI Makefile,v 2.1 1995/02/03 13:00:05 polk Exp

.include "../../Makefile.inc"

MAN1=	rcsfreeze.0

all rcsfreeze: ${MAN1}

clean depend lint tags:

cleandir:
	rm -f ${MAN1}

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/rcsfreeze.sh ${DESTDIR}/${BINDIR}/rcsfreeze

.include <bsd.prog.mk>
.include <bsd.man.mk>

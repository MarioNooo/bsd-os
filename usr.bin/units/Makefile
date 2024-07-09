#	BSDI Makefile,v 2.1 1995/02/03 13:15:00 polk Exp

MAN1=	units.0

all units: ${MAN1}

clean depend lint tags:

cleandir:
	rm -f ${MAN1}

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/units.pl ${DESTDIR}/${BINDIR}/units

.include <bsd.prog.mk>
.include <bsd.man.mk>

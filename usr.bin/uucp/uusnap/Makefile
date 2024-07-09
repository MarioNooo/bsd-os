# Makefile,v 2.1 1995/02/03 13:22:33 polk Exp

BINDIR=	/usr/bin
BINMODE=6555

MAN1=	uusnap.0

all uusnap: ${MAN1}

clean depend lint tags:

cleandir:
	rm -f ${MAN1}

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/uusnap.pl ${DESTDIR}${BINDIR}/uusnap

.include <bsd.prog.mk>
.include <bsd.man.mk>

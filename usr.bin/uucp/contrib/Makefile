# Makefile,v 2.1 1998/12/19 00:56:48 tks Exp

BINDIR=	/etc/uucp
BINMODE=0550

MAN8=	uupoll.0

all uupoll: ${MAN8}

clean depend lint tags:

cleandir:
	rm -f ${MAN8}

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/uupoll ${DESTDIR}${BINDIR}/uupoll
	install -c -o ${BINOWN} -g ${BINGRP} -m 0640 \
	    ${.CURDIR}/Poll ${DESTDIR}${BINDIR}/Poll

.include <bsd.prog.mk>
.include <bsd.man.mk>

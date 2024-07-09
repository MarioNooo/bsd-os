#	BSDI Makefile,v 2.1 1995/02/03 15:16:40 polk Exp

all clean cleandir depend lint tags:

SCRIPTS=custom fixperm
BINDIR=/sco/etc

install:
	for i in ${SCRIPTS}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		    ${.CURDIR}/$$i ${DESTDIR}/${BINDIR}/$$i; \
	done

.include <bsd.prog.mk>

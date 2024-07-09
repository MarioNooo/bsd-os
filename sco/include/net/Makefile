#	BSDI Makefile,v 1.1 1995/07/10 18:24:18 donn Exp

all clean cleandir depend lint tags:

IDIR=/sco/usr/include/net
HEADERS= \
	errno.h

beforeinstall:
	@-mkdir -p ${DESTDIR}${IDIR}
	@-chmod 755 ${DESTDIR}${IDIR}
	@-set -x; for f in ${HEADERS}; do \
		cmp -s ${.CURDIR}/$$f ${DESTDIR}${IDIR}/$$f || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/$$f \
			${DESTDIR}${IDIR}/$$f; \
	done

.include <bsd.prog.mk>

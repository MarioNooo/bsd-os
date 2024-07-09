#	BSDI Makefile,v 2.2 1995/07/10 17:51:28 donn Exp

PROG=echo
NOMAN=noman

SCRIPTS=cc cpio cpp df expr pg ps ulimit uname unpack

afterinstall:
	for i in ${SCRIPTS}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		    ${.CURDIR}/$$i ${DESTDIR}${BINDIR}/$$i; \
	done
	-@rm -f ${DESTDIR}${BINDIR}/gcc
	ln ${DESTDIR}${BINDIR}/cc ${DESTDIR}${BINDIR}/gcc

.include <bsd.prog.mk>

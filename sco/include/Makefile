#	BSDI Makefile,v 1.1 1995/07/10 18:20:40 donn Exp

all clean cleandir depend lint tags:

SUBDIR=machine net netinet sys

IDIR=/sco/usr/include
HEADERS= \
	ctype.h errno.h fcntl.h limits.h math.h memory.h netdb.h \
	pwd.h signal.h stdarg.h stddef.h stdio.h string.h time.h \
	unistd.h varargs.h

beforeinstall:
	@-mkdir -p ${DESTDIR}${IDIR}
	@-chmod 755 ${DESTDIR}${IDIR}
	@-set -x; for f in ${HEADERS}; do \
		cmp -s ${.CURDIR}/$$f ${DESTDIR}${IDIR}/$$f || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/$$f \
			${DESTDIR}${IDIR}/$$f; \
	done

.include <bsd.prog.mk>

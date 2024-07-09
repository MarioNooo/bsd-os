#	BSDI Makefile,v 2.4 2003/06/05 23:54:06 polk Exp
#	@(#)Makefile	8.1 (Berkeley) 6/7/93

PROG=	mklocale
SRCS=	yacc.c lex.c
CFLAGS+=-I. -I${.CURDIR}
CLEANFILES+=y.tab.h yacc.c lex.c

LOCALE=Japanese-EUC POSIX-UTF2 ISO8859-1

afterinstall:
	@for f in ${LOCALE}; do \
		mklocale < ${.CURDIR}/$$f > \
			${DESTDIR}/usr/share/locale/$$f/LC_CTYPE; \
		chmod 0444 ${DESTDIR}/usr/share/locale/$$f/LC_CTYPE; \
		chown ${BINOWN}.${BINGRP} \
			${DESTDIR}/usr/share/locale/$$f/LC_CTYPE; \
	done

.include <bsd.prog.mk>

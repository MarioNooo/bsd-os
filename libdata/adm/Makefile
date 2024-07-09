#	BSDI Makefile,v 1.3 2002/01/13 22:58:54 polk Exp

FILES=	adm.pl netutil.pl query.pl util.pl v.pl x.pl Descriptions.pl

tags depend:

install: beforeinstall

beforeinstall:
	for f in ${FILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			${.CURDIR}/$$f ${DESTDIR}/${BINDIR}/adm; \
	done

.include <bsd.prog.mk>

#	BSDI	Makefile,v 2.1 1995/02/03 16:19:40 polk Exp
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

#missing: tmac.r
FILES=	tmac.andoc tmac.s tmac.e man.local
MSRCS=  doc doc-ditroff doc-common doc-nroff doc-syms

LINKS=	${BINDIR}/tmac/tmac.andoc ${BINDIR}/tmac/tmac.an
NOOBJ=	noobj

# old macros not currently installed...
# OFILES=	tmac.a tmac.cp

all clean cleandir depend lint tags:

beforeinstall:
	for i in ${MSRCS}; do \
		sed -f strip.sed < $$i > ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
		chown ${BINOWN}.${BINGRP} ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
		chmod 444 ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
	done
	install -c -o ${BINOWN} -g ${BINOWN} -m 444 ${FILES} \
		${DESTDIR}${BINDIR}/tmac


.include <bsd.prog.mk>

#	BSDI	Makefile,v 1.1 1997/11/21 17:05:41 geertj Exp
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

FILES=	cy.doc
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/doc/cyclades

.include "../../Makefile.inc"
.include <bsd.prog.mk>

#	BSDI	Makefile,v 1.1 1996/12/02 17:35:51 cp Exp
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

FILES=	eqnx.doc
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/doc/equinox

.include "../../Makefile.inc"
.include <bsd.prog.mk>

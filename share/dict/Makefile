#	BSDI	Makefile,v 2.1 1995/02/03 15:44:15 polk Exp
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

# missing: connectives
FILES=	README propernames web2 web2a words
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/dict

.include <bsd.prog.mk>

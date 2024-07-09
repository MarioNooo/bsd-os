#	BSDI Makefile,v 2.2 2003/07/08 21:56:06 polk Exp

.include "../Makefile.defs"

PROG=	indxbib
SRCS=	dirnamemax.c indxbib.cc signal.c
LDADD+=	${LIBBIB} ${LIBGROFF} -lm
DPADD+=	${LIBBIB} ${LIBGROFF} ${LIBM}

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/eign \
		${DESTDIR}${FONTDIR}/

.include "../Makefile.common"
.include <bsd.prog.mk>

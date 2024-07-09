#	BSDI Makefile,v 2.2 2003/07/08 21:56:05 polk Exp

.include "../Makefile.defs"

PROG=	groff
SRCS=	groff.cc pipeline.c
LDADD+=	${LIBGROFF} ${LIBM}
DPADD+=	${LIBGROFF} ${LIBM}
MLINKS+=groff.1 psroff.1 groff.1 xroff.1

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/psroff ${.CURDIR}/xroff ${DESTDIR}${BINDIR}

.include "../Makefile.common"
.include <bsd.prog.mk>

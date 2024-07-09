#	BSDI Makefile,v 2.2 2003/07/08 21:56:05 polk Exp

.include "../Makefile.defs"

PROG=	grotty
SRCS=   tty.cc
LDADD+=	${LIBDRIVER} ${LIBGROFF} ${LIBM}
DPADD+=	${LIBDRIVER} ${LIBGROFF} ${LIBM}

.include "../Makefile.common"
.include <bsd.prog.mk>

#	BSDI Makefile,v 2.2 2003/07/08 21:56:06 polk Exp

.include "../Makefile.defs"

PROG=	lookbib
SRCS=	lookbib.cc
LDADD+=	${LIBBIB} ${LIBGROFF} ${LIBM}
DPADD+=	${LIBBIB} ${LIBGROFF} ${LIBM}

.include "../Makefile.common"
.include <bsd.prog.mk>

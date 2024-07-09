#	BSDI Makefile,v 2.2 2003/07/08 21:56:06 polk Exp

.include "../Makefile.defs"

PROG=	refer
SRCS=   label.cc command.cc ref.cc refer.cc token.cc
LDADD+=	${LIBBIB} ${LIBGROFF} ${LIBM}
DPADD+=	${LIBBIB} ${LIBGROFF} ${LIBM}
CLEANFILES+=label.tab.h label.cc

.include "../Makefile.common"
.include <bsd.prog.mk>

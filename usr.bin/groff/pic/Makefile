#	BSDI Makefile,v 2.2 2003/07/08 21:56:06 polk Exp

.include "../Makefile.defs"

PROG=	pic
SRCS=   pic.cc common.cc lex.cc main.cc object.cc tex.cc troff.cc
LDADD+=	${LIBGROFF} ${LIBM}
DPADD+=	${LIBGROFF} ${LIBM}
CLEANFILES+=pic.tab.h pic.cc

.depend: pic.tab.h

pic.tab.h: pic.cc

.include "../Makefile.common"
.include <bsd.prog.mk>

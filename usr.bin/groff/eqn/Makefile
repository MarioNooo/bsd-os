#	BSDI Makefile,v 2.3 2003/07/08 21:56:05 polk Exp

.include "../Makefile.defs"

PROG=	eqn
SRCS=   eqn.cc box.cc delim.cc lex.cc limit.cc list.cc main.cc mark.cc \
	other.cc over.cc pile.cc script.cc special.cc sqrt.cc text.cc
LDADD+=	${LIBGROFF}
DPADD+=	${LIBGROFF}
CLEANFILES+=eqn.tab.h eqn.cc eqn.c

.depend: eqn.tab.h

eqn.tab.h: eqn.cc

.include "../Makefile.common"
.include <bsd.prog.mk>

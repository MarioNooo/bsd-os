#	BSDI Makefile,v 2.1 1995/02/03 12:58:11 polk Exp

.include "../../Makefile.inc"

PROG=	co
CFLAGS+=-I${.CURDIR}/../lib
LDADD+=	${.CURDIR}/../lib/obj/librcs.a
DPADD+=	${.CURDIR}/../lib/obj/librcs.a

.include <bsd.prog.mk>

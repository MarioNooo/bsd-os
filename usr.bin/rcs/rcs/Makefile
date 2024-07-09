#	BSDI Makefile,v 2.1 1995/02/03 12:59:37 polk Exp

.include "../../Makefile.inc"

PROG=	rcs
CFLAGS+=-I${.CURDIR}/../lib
LDADD+=	${.CURDIR}/../lib/obj/librcs.a
DPADD+=	${.CURDIR}/../lib/obj/librcs.a

MAN1=rcs.0 rcsintro.0
MAN5=rcsfile.0

.include <bsd.prog.mk>

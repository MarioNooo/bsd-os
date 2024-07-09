#	BSDI Makefile,v 2.1 1995/02/03 12:59:30 polk Exp

.include "../../Makefile.inc"

PROG=	merge
SRCS=	merge.c merger.c
CFLAGS+=-I${.CURDIR}/../lib
LDADD+=	${.CURDIR}/../lib/obj/librcs.a
DPADD+=	${.CURDIR}/../lib/obj/librcs.a

.include <bsd.prog.mk>

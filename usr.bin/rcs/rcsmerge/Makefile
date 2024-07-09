#	BSDI Makefile,v 2.1 1995/02/03 13:00:10 polk Exp

.include "../../Makefile.inc"

PROG=	rcsmerge
SRCS=	rcsmerge.c merger.c
CFLAGS+=-I${.CURDIR}/../lib
LDADD+=	${.CURDIR}/../lib/obj/librcs.a
DPADD+=	${.CURDIR}/../lib/obj/librcs.a

.PATH:	${.CURDIR}/../merge

.include <bsd.prog.mk>

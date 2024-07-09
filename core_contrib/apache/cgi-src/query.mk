#	BSDI	query.mk,v 1.4 2001/02/26 20:49:21 polk Exp
#
PROG=query
SRCS=query.c util.c
NOMAN=noman

.include <bsd.prog.mk>
BINDIR=${WWWCGIDIR}

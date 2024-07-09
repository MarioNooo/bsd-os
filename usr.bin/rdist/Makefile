#	@(#)Makefile	8.3 (Berkeley) 7/19/93

PROG=	rdist
SRCS=	docmd.c expand.c gram.c lookup.c main.c server.c
CFLAGS+=-I. -I${.CURDIR}
CLEANFILES=gram.c y.tab.h

.include <bsd.prog.mk>

# WILDBOAR $Wildboar: Makefile,v 1.1 1996/03/17 08:31:55 shigeya Exp $

PROG=	csctl
SRCS=	csctl.c kernio.c parse.c param.c event.c daemon.c
CFLAGS+=-I/sys

LDADD+=-lcs

MAN8=	csctl.0

.include <bsd.prog.mk>

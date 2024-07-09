#	BSDI Makefile,v 1.6 2003/09/15 18:11:04 jch Exp

PROG=		transform

SRCS=           config.c cookie.c decl.c flag.c func.c main.c \
		parse.y scan.l struct.c transform.c type.c typedef.c \
		linux.c
CFLAGS+=	-g -I${.CURDIR} -W -Wall -Wshadow -Wundef -Wpointer-arith \
		-Wstrict-prototypes -Wmissing-prototypes -Wnested-externs
LDADD+=		-g -ll
NOMAN=		noman
CLEANFILES+=	y.tab.h

install:

.include <bsd.prog.mk>

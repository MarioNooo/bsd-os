#	BSDI Makefile,v 2.1 1995/02/03 05:58:05 polk Exp
#	@(#)Makefile	8.1 (Berkeley) 6/12/93

PROG=	adventure
SRCS=	main.c init.c done.c save.c subr.c vocab.c wizard.c io.c data.c crc.c
MAN6=	adventure.0
HIDEGAME=hidegame
CLEANFILES+=setup data.c

data.c: glorkz setup
	./setup ${.CURDIR}/glorkz > data.c

setup: setup.c hdr.h
	${CC} -o setup ${.CURDIR}/setup.c

.include <bsd.prog.mk>

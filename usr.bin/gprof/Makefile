#	BSDI Makefile,v 2.2 1998/09/10 20:21:49 torek Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/29/93

.if ${MACHINE} != "sparc_v9"
MCH=	${MACHINE}
.else
MCH=	sparc
.endif

PROG=	gprof
SRCS=	gprof.c arcs.c dfn.c lookup.c ${MCH}.c hertz.c \
	printgprof.c printlist.c
CFLAGS+=-I.
CLEANFILES=machine.h

machine.h:
	ln -s ${.CURDIR}/${MCH}.h machine.h

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${.CURDIR}/gprof.flat ${.CURDIR}/gprof.callg \
	    ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>

.depend: machine.h

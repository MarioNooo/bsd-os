#	BSDI Makefile,v 2.5 1997/12/21 02:13:56 donn Exp

PROG=libc_s
SRCS=	libc_jump.s libc_data.s bsd_calls.s calloc.c malloc.c sco_calls.s \
	sco_compat.c stdio_compat.c sys_compat.c getopt.c
LDFLAGS=-static -Ttext a0000000 -Tdata a0400000

BINDIR=/sco/shlib
CPPFLAGS+=-I${.CURDIR}/../../syscalls
CFLAGS+=${CPPFLAGS}

NOMAN=noman

libc_s: ${OBJS} ${LIBCOMPAT} ${LIBC}
	${LD} ${LDFLAGS} -o ${.TARGET} ${OBJS} -lcompat -lc

.include <bsd.prog.mk>

.s.o:
	${CPP} ${CPPFLAGS} -I${.CURDIR}/../syscalls ${.IMPSRC} | \
		as ${AFLAGS} -o ${.TARGET}

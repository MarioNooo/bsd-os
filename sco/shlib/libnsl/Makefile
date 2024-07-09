#	BSDI Makefile,v 2.4 1997/12/21 02:13:56 donn Exp

PROG=libnsl_s
SRCS=	libnsl_jump.s libnsl_data.s bsd_calls.s \
	nsl_compat.c nsl_messages.c nsl_debug.c \
	nsl_errno.c \
	t_accept.c \
	t_alloc.c \
	t_bind.c \
	t_close.c \
	t_connect.c \
	t_error.c \
	t_free.c \
	t_getinfo.c \
	t_getstate.c \
	t_listen.c \
	t_look.c \
	t_open.c \
	t_optmgmt.c \
	t_rcv.c \
	t_rcvconnect.c \
	t_rcvdis.c \
	t_rcvrel.c \
	t_rcvudata.c \
	t_rcvuderr.c \
	t_snd.c \
	t_snddis.c \
	t_sndrel.c \
	t_sndudata.c \
	t_sync.c \
	t_unbind.c \

LDFLAGS=-static -Ttext a1000000 -Tdata a1400000

BINDIR=/sco/shlib
CPPFLAGS+=-I${.CURDIR}/../../syscalls
CFLAGS+=${CPPFLAGS}

NOMAN=noman

libnsl_s: ${OBJS} ${LIBCOMPAT} ${LIBC}
	${LD} ${LDFLAGS} -o ${.TARGET} ${OBJS} -lc

.include <bsd.prog.mk>

.s.o:
	${CPP} ${CPPFLAGS} -I${.CURDIR}/../syscalls ${.IMPSRC} | \
		as ${AFLAGS} -o ${.TARGET}

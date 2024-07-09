#	BSDI Makefile,v 2.6 1997/12/21 02:13:55 donn Exp

EMULATE_SRCS=\
	emulate_glue.s emulate_break.c emulate_start.c emulate_bits.c \
	emulate_smalloc.c
BSD_SRCS=\
	bsd_transform.c bsd_genericops.c
SCO_SRCS=\
	sco_transform.c \
	sco_glue.s sco_coff.c \
	sco_genericops.c sco_regops.c sco_dirops.c sco_sockops.c sco_ttyops.c \
	sco_break.c sco_conf.c sco_errno.c sco_fork.c sco_getpid.c \
	sco_getuid.c sco_ipc.c sco_mknod.c sco_open.c sco_poll.c \
	sco_prot.c sco_signal.c sco_stat.c sco_statfs.c sco_sysi86.c \
	sco_ulimit.c sco_uname.c sco_wait.c
HEADERS=\
	emulate.h sco.h sco_errno.h sco_ops.h sco_sig_state.h

PROG=	emulator
SRCS=	${EMULATE_SRCS} ${BSD_SRCS} ${SCO_SRCS}
LDFLAGS=-static -Ttext c0001000 -Tdata c0400000 -e start
CPPFLAGS=-I${.CURDIR} -DDEBUG
CFLAGS+=${CPPFLAGS}
AFLAGS=
CLEANFILES+=bsd_transform.c sco_transform.c
BINDIR=	/sco
BINOWN=	root
BINMODE=0555

MAN5=	ibcs2.0

emulator: ${EMULATE_SRCS}
	${LD} ${LDFLAGS} -o ${.TARGET} ${OBJS} \
		${.CURDIR}/../syscalls/obj/syscalls.o -lcompat -lc

sco_transform.c: transform.awk sco_transform.x sco_transform2.x emulate.h
	${.CURDIR}/transform.awk ${.CURDIR}/sco_transform.x \
		${.CURDIR}/sco_transform2.x > sco_transform.tmp
	mv sco_transform.tmp sco_transform.c
sco_transform.o: transform.awk sco_transform.c sco_transform.x \
	sco_transform2.x emulate.h

bsd_transform.c: transform.awk bsd_transform.x emulate.h
	${.CURDIR}/transform.awk ${.CURDIR}/bsd_transform.x > bsd_transform.tmp
	mv bsd_transform.tmp bsd_transform.c
bsd_transform.o: transform.awk bsd_transform.c bsd_transform.x emulate.h

tags: ${SRCS} ${HEADERS}
	ctags -dtw ${.ALLSRC}
	mv tags ${.CURDIR}

# don't copy onto old image
beforeinstall:
	rm -f ${DESTDIR}${BINDIR}/emulator

.include <bsd.prog.mk>

.s.o:
	${CPP} ${CPPFLAGS} -I${.CURDIR}/../syscalls ${.IMPSRC} | \
		as ${AFLAGS} -o ${.TARGET}

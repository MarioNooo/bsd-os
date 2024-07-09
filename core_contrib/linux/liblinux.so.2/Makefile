#	BSDI Makefile,v 1.15 2003/09/15 19:37:34 jch Exp

PROG=		liblinux.so.2

SRCS=           brk.x dirent.x dirent2.x errno.x fcntl.x ioctl.x \
		ipc.x mknod.x mman.x mman64.x personality.x \
		resource.x sched.x select.x signal.x signal2.x socket.x \
		stat.x stat64.x statfs.x stdio.x termios.x time.x uio.x \
		uname.x unistd.x unistd2.x vfork.x wait.x \
		bsdi.s glue.s trace.x unimpl.c

NOMAN=		noman

LDCC?=		cc
CFLAGS+=	-g
TFLAGS=		-k -i ${.CURDIR} -i ${.CURDIR}/../liblinux.common \
		-d ${.CURDIR}/../afdb/obj/libc-2.1.3.so

BINDIR=		${DESTDIR}/linux/lib
BINMODE=	444
STRIP=

CLEANFILES+=	${SRCS:M*.x:S/.x$/.c/g}

glue.s: mkglue ${.CURDIR}/../D/asm/unistd.h
	sh ${.CURDIR}/mkglue ${.CURDIR}/../D/asm/unistd.h > ${.TARGET}
CLEANFILES+=	glue.s

unimpl.c: mkunimpl ${.CURDIR}/../D/asm/unistd.h
	sh ${.CURDIR}/mkunimpl ${.CURDIR}/../D/asm/unistd.h > ${.TARGET}
CLEANFILES+=	unimpl.c

# Hack: don't use the BSD libgcc.so.
trap.o: libc.a libgcc.a
libc.a libgcc.a:
	as -o ${.TARGET} /dev/null
CLEANFILES+=libc.a libgcc.a

# Hack: binutils ld screws up without MALLOC_OPTIONS=Z.
${PROG}: ${OBJS}
	MALLOC_OPTIONS=Z \
	${LDCC} -shared -nodefaultlibs -o ${PROG} \
		-Wl,-h,${PROG} -Wl,--version-script=${.CURDIR}/Versions \
		-L${.OBJDIR} -L${BINDIR} ${OBJS} -lc-2.1.3

.include <bsd.prog.mk>

.PATH:		${.CURDIR}/../liblinux.common

TRANSFORM=	${.CURDIR}/../transform/obj/transform

.SUFFIXES: .x

.x.c:
	${TRANSFORM} ${TFLAGS} ${.IMPSRC}

.x.o:
	${TRANSFORM} ${TFLAGS} ${.IMPSRC}
	${CC} -I${.CURDIR} -I${.CURDIR}/../liblinux.common \
		-fPIC ${CFLAGS} -c ${.PREFIX}.c || \
		(rm -f ${.PREFIX}.c; false)
	rm -f ${.PREFIX}.c

.c.o:
	${CC} -fPIC ${CFLAGS} -c ${.IMPSRC}

.s.o:
	${CPP} -DLOCORE -D__PIC__ ${.IMPSRC} | as -o ${.TARGET}

#	BSDI Makefile,v 1.16 2003/09/24 21:28:55 polk Exp


PROG=		liblinux.so.1

DIRSRCS=        closedir.x directory.x getdents.x opendir.x readdir.x \
		rewinddir.x seekdir.x telldir.x
SRCS=           brk.x dirent.x errno.x fcntl.x ioctl.x ipc.x \
		mknod.x mman.x resource.x select.x signal.x socket.x stat.x \
		statfs.x stdio.x termios.x time.x uio.x uname.x unistd.x \
		wait.x unimpl.x bsdi.s trap.x trace.x ${DIRSRCS}

NOMAN=		noman

LDCC?=		cc
CFLAGS+=	-g -DLIBC5=1 -I${.CURDIR}
CFLAGS+=	-I${.CURDIR}/../../../lib/libc/threads \
		-I${.CURDIR}/../../../lib/libc/${MACHINE}/threads \
		-I${.CURDIR}/../liblinux.common
TFLAGS=		-i ${.CURDIR} -i ${.CURDIR}/../liblinux.common \
		-d ${.CURDIR}/../afdb/obj/libc.so.5.3.12

BINDIR=		${DESTDIR}/linux/usr/i486-linux-libc5/lib
BINMODE=	444
STRIP=

CLEANFILES+=	${SRCS:M*.x:S/.x$/.c/g}

# Hack: don't use the defaulted BSD libraries when linking with cc.
trap.o: libc.a libgcc.a
libc.a libgcc.a:
	as -o ${.TARGET} /dev/null
LDDYNDEP+=-L${.OBJDIR}
CLEANFILES+=libc.a libgcc.a

${PROG}: ${OBJS}
	${LDCC} -shared -o ${PROG} -Wl,-h,${PROG} \
		-L${.OBJDIR} ${OBJS} ${BINDIR}/libc.so.5

depend:

.include <bsd.prog.mk>

.PATH:		${.CURDIR}/../liblinux.common

TRANSFORM=	${.CURDIR}/../transform/obj/transform

.SUFFIXES: .x

.x.c:
	${TRANSFORM} ${TFLAGS} ${.IMPSRC}

.x.o:
	${TRANSFORM} ${TFLAGS} ${.IMPSRC}
	${CC} -fPIC ${CFLAGS} -c ${.PREFIX}.c || \
		(rm -f ${.PREFIX}.c; false)
	rm -f ${.PREFIX}.c

.c.o:
	${CC} -fPIC ${CFLAGS} -c ${.IMPSRC}

.s.o:
	${CPP} -DLIBC5=1 -DLOCORE -D__PIC__ ${.IMPSRC} | as -o ${.TARGET}

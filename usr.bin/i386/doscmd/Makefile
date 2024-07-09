# BSDI Makefile,v 2.12 1997/12/09 16:49:39 donn Exp

PROG=	doscmd
SRCS=	AsyncIO.c ParseBuffer.c \
	bios.c cmos.c config.c cwd.c device.c disktab.c doscmd.c exe.c \
	i386-pinsn.c inout.s \
	int10.c int11.c int12.c int13.c int14.c int15.c int16.c int17.c \
	int1a.c int20.c int21.c int2f.c int33.c intff.c \
	mem.c port.c setver.c signal.c trace.c trap.c tty.c

BINGRP=	kmem
EXEGRP=	bin
BINMODE=2555
EXEMODE=444
CFLAGS+=-I/sys -I/usr/X11/include -I. -DDISASSEMBLER
LDFLAGS+=-L/usr/X11/lib
LDADD+=	-lX11 -lc
DPADD+=	/usr/X11/lib/libX11.a ${LIBC}
CLEANFILES+=crt0.o doscmd.kernel doscmd_loader.o

afterinstall:
	install ${COPY} -o ${BINOWN} -g ${EXEGRP} -m ${EXEMODE} \
		doscmd.kernel ${DESTDIR}/usr/libexec/doscmd.kernel
	install -c -o ${BINOWN} -g ${EXEGRP} -m ${EXEMODE} \
		${.CURDIR}/instbsdi.exe ${DESTDIR}/usr/share/misc/

doscmd: doscmd.kernel doscmd_loader.o
	${CC} -static -N -Wl,-m,i386bsdi -Wl,-e,_start -o doscmd \
		doscmd_loader.o

.include <bsd.prog.mk>

.depend: doscmd_loader.c

doscmd.kernel: crt0.o ${OBJS}
	ld -m i386bsdi -e start -static -N -Ttext 110000 -o doscmd.kernel \
		${LDFLAGS} crt0.o ${OBJS} ${LDADD}

doscmd.0: doscmd.1
	tbl ${.IMPSRC} | nroff -mandoc > ${.TARGET}

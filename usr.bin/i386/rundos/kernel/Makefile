# BSDI Makefile,v 2.6 1998/01/12 23:30:56 polk Exp

PROG=	kernel
SRCS=	kernel.c emulate.c service.c config.c queue.c \
	keyboard.c console.c interrupt.c timer.c cmos.c trace.c int13.c \
	video.c menu.c printer.c mouse.c xms.c input.c
NOMAN=noman

CFLAGS+=-I/sys

.include "../Makefile.inc"

kernel: crt0.o $(OBJS)
	ld -m i386bsdi -static -e start -N -Ttext 110000 -o kernel \
		crt0.o $(OBJS) -lc

install:
	install -c -s kernel ${DESTDIR}${KERNEL_FILE_NAME}

.include <bsd.prog.mk>

#	BSDI Makefile,v 1.7 2001/11/05 23:06:41 polk Exp

PROG=	cpu
CFLAGS= -g -I../../../sys -Wall
MAN5=	mp.config.0
MAN8=	cpu.0
SRCS=	bootparam.c cpu.c cpucheck.c intr.c intr_clock.c intr_debug.c \
	intr_special.c intr_util.c kerninfo.c map.c misc.c mpcpu.c \
	mpinfo.c override.c pci.c siomode.c

.include <bsd.prog.mk>

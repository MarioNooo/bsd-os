#	BSDI Makefile,v 2.5 1998/09/10 21:26:32 torek Exp

.if ${MACHINE} != "sparc_v9"
MCH=	${MACHINE}
.else
MCH=	sparc
.endif

PROG=	shlist
SRCS=	aout.c elf32.c elf64.c shlist.c ${MCH}.c
MAN8=	shlist.0

.include <bsd.prog.mk>

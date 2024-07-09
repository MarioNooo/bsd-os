#	BSDI Makefile.inc,v 2.5 1998/09/21 13:34:47 torek Exp
#
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# machine-dependent gen sources
SRCS+=	_setjmp.s alloca.s fabs.s fixunsdfsi.s modf.s mul.s saveregs.s \
	setjmp.s sigsetjmp.s umul.s
SRCS+=	frexp.c isinf.c isnan.c ldexp.c

# Turn on ELF64 support in nlist code.
CFLAGS+=-DNLIST64

# for Q_*.c
CFLAGS+=-mhard-quad-float
SRCS+=	Q_add.c Q_sub.c Q_neg.c Q_mul.c Q_div.c \
	Q_itoq.c Q_qtoi.c Q_qtos.c Q_qtou.c Q_stoq.c Q_dtoq.c Q_qtod.c \
	Q_feq.c Q_fne.c Q_fgt.c Q_fge.c Q_flt.c Q_fle.c \
	Q_sqrt.c

# `source' files built from m4 source
# the name `div.o' is taken for the ANSI C `div' function, hence sdiv here
SRCS+=	rem.s sdiv.s udiv.s urem.s
CLEANFILES+=rem.s sdiv.s udiv.s urem.s

# these are copied down to the kernel library
KMSRCS+=mul.s rem.s sdiv.s udiv.s umul.s urem.s

# need DEFS.h too
libkern.sparc::
	cp -p ${.CURDIR}/sparc/DEFS.h /sys/libkern/sparc

sdiv.s: divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@rm -f ${.TARGET}
	@(echo "define(NAME,\`.div')define(OP,\`div')define(S,\`true')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	@chmod 444 ${.TARGET}

udiv.s: divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@rm -f ${.TARGET}
	@(echo "define(NAME,\`.udiv')define(OP,\`div')define(S,\`false')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	@chmod 444 ${.TARGET}

rem.s: divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@rm -f ${.TARGET}
	@(echo "define(NAME,\`.rem')define(OP,\`rem')define(S,\`true')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	@chmod 444 ${.TARGET}

urem.s: divrem.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@rm -f ${.TARGET}
	@(echo "define(NAME,\`.urem')define(OP,\`rem')define(S,\`false')"; \
	 cat ${.ALLSRC}) | m4 > ${.TARGET}
	@chmod 444 ${.TARGET}

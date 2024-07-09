#	BSDI Makefile,v 2.4 1995/11/23 00:19:41 torek Exp
#	from @(#)Makefile	5.3 (Berkeley) 5/11/90

PROG=	diskdefect
OPS=	bad144.c null.c scsi.c
SRCS=	${OPS} diskdefect.c ops.c opstab.c scan.c util.c
MAN8=	diskdefect.0
MAN5=	bad144.0
DPADD=	${LIBM}
LDADD=	-lm

# There is no machine-dependent code in here now...
# perhaps we should take out the MANSUBDIR

MANSUBDIR=/${MACHINE}

.include <bsd.prog.mk>

# BSDI Makefile,v 1.1 1996/09/25 01:22:38 prb Exp

PROG=	configure_system
SRCS=
STRIP=
NOMAN=
depend tags:

${PROG}: ${.CURDIR}/${PROG}.pl
	cp ${.CURDIR}/${PROG}.pl ./${PROG}
	chmod +x ./${PROG}

.include <bsd.prog.mk>

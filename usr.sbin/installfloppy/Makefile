#	BSDI Makefile,v 1.2 1996/09/21 17:42:47 prb Exp

PROG=	installfloppy
MAN8=	${PROG}.0
SRCS=
STRIP=
depend tags:

${PROG}: ${.CURDIR}/${PROG}.pl
	cp ${.CURDIR}/${PROG}.pl ./${PROG}
	chmod +x ./${PROG}

.include <bsd.prog.mk>

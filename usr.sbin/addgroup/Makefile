# BSDI Makefile,v 1.2 1996/09/17 01:43:12 prb Exp

PROG=	addgroup
SRCS=
STRIP=
MAN8=	${PROG}.0
MLINKS=	addgroup.8 rmgroup.8
LINKS=	${BINDIR}/addgroup ${BINDIR}/rmgroup
depend tags:

${PROG}: ${.CURDIR}/${PROG}.pl
	cp ${.CURDIR}/${PROG}.pl ./${PROG}
	chmod +x ./${PROG}

.include <bsd.prog.mk>

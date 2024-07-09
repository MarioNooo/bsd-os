#	BSDI	Makefile,v 1.1 1996/10/11 22:55:10 sanders Exp

PROG=	config_X
SRCS=
STRIP=
MAN8=	${PROG}.0

depend tags:

${PROG}: ${.CURDIR}/${PROG}.pl
	cp ${.CURDIR}/${PROG}.pl ./${PROG}
	chmod +x ./${PROG}

.include <bsd.prog.mk>

#	BSDI	Makefile,v 1.2 1996/09/30 19:18:12 sanders Exp

PROG=	maxim
SRCS=
STRIP=
MAN8=	${PROG}.0

SUBDIR?=maxim_init

depend tags:

${PROG}: ${.CURDIR}/${PROG}.sh
	cp ${.CURDIR}/${PROG}.sh ./${PROG}
	chmod +x ./${PROG}

.include <bsd.prog.mk>

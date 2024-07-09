# BSDI Makefile,v 2.2 1996/11/14 18:20:53 polk Exp

PROG=	config_anonftp
SRCS=
STRIP=
MAN8=   ${PROG}.0
depend tags:

${PROG}: ${.CURDIR}/${PROG}.pl
	cp ${.CURDIR}/${PROG}.pl ./${PROG}
	chmod +x ./${PROG}

.include <bsd.prog.mk>

# BSDI Makefile,v 1.3 1996/09/17 01:42:38 prb Exp

PROG=	adduser
SRCS=
STRIP=
MAN8=	${PROG}.0
MLINKS=	adduser.8 rmuser.8
LINKS=	${BINDIR}/adduser ${BINDIR}/rmuser
depend tags:

${PROG}: ${.CURDIR}/${PROG}.pl
	cp ${.CURDIR}/${PROG}.pl ./${PROG}
	chmod +x ./${PROG}

afterinstall:
	install -c -o root -g ${BINGRP} -m 640 ${.CURDIR}/passwd.cf \
		${DESTDIR}/var/db

.include <bsd.prog.mk>

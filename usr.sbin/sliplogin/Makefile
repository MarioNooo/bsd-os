#	BSDI Makefile,v 2.2 2002/02/27 23:35:00 polk Exp
#
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	sliplogin
MAN8=	sliplogin.0
BINOWN=	root
BINGRP=	netdial
BINMODE=4550

afterinstall:
	(cd ${.CURDIR}; install -c -o ${BINOWN} -g ${BINGRP} -m 0755 \
			slip.login slip.logout ${DESTDIR}/etc/sliphome)
	(cd ${.CURDIR}; install -c -o ${BINOWN} -g ${BINGRP} -m 0640 \
			slip.hosts ${DESTDIR}/etc/sliphome)

.include <bsd.prog.mk>

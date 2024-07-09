#	BSDI	Makefile,v 2.2 1995/10/10 14:21:06 bostic Exp

MAN8=	chkconfig.0

chkconfig clean depend lint tags:

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/chkconfig.sh ${DESTDIR}/sbin/chkconfig

.include <bsd.prog.mk>

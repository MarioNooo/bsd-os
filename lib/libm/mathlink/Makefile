#	BSDI Makefile,v 1.2 1998/03/04 18:41:36 donn Exp

MAN8=	mathlink.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/mathlink.${MACHINE}.sh ${DESTDIR}/${BINDIR}/mathlink

.include <bsd.prog.mk>

MAN1=	pagesize.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/pagesize.sh ${DESTDIR}/${BINDIR}/pagesize

.include <bsd.prog.mk>

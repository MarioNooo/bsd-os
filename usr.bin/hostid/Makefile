MAN1=	hostid.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/hostid.sh ${DESTDIR}/${BINDIR}/hostid

.include <bsd.prog.mk>

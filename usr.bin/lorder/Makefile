MAN1=	lorder.0

afterinstall:
	-if [ -r ${.CURDIR}/lorder.${MACHINE}.sh ]; then \
		install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		    ${.CURDIR}/lorder.${MACHINE}.sh \
		    ${DESTDIR}/${BINDIR}/lorder; \
	else \
		install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		    ${.CURDIR}/lorder.sh ${DESTDIR}/${BINDIR}/lorder; \
	fi

.include <bsd.prog.mk>

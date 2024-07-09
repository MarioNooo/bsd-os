#	@(#)Makefile	8.1 (Berkeley) 6/6/93

MAN1=	mkdep.0

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/mkdep.gcc.sh ${DESTDIR}/usr/bin/mkdep

# old (pre-gcc) version of mkdep, preserved for historical reasons
# beforeinstall:
#	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
#	    ${.CURDIR}/mkdep.sh ${DESTDIR}/usr/bin/mkdep

.include <bsd.prog.mk>

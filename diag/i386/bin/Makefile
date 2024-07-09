
FILES=memtest.image README.memtest
TFILES=README
SRCS=
STRIP=
NOMAN=1

.include <bsd.prog.mk>

install:
	(cd ${.CURDIR}; install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${INSTALLFLAGS} ${FILES} ${DESTDIR}${BINDIR})
	(cd ${.CURDIR}; install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${INSTALLFLAGS} ${TFILES} ${DESTDIR}${BINDIR}/..)

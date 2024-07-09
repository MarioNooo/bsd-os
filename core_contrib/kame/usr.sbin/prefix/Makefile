
PROG=	prefix

MAN8=	prefix.0

realinstall:
	install $(COPY) -o $(BINOWN) -g $(BINGRP) -m 555 ${.CURDIR}/prefix.sh \
		${DESTDIR}${BINDIR}/prefix

.include <bsd.prog.mk>

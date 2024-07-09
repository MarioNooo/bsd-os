MAN8=	makewhatis.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 755 ${.CURDIR}/makewhatis \
	    ${DESTDIR}/usr/sbin/makewhatis
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/makewhatis.sed \
	    ${DESTDIR}/usr/share/man/makewhatis.sed
	install -c -o ${BINOWN} -g ${BINGRP} -m 555 ${.CURDIR}/makewhatis.perl \
	    ${DESTDIR}/usr/share/man/makewhatis.perl

.include <bsd.prog.mk>

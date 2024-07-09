MAN8=	config_www.0

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/config_www.pl ${DESTDIR}/${BINDIR}/config_www
	install -c -o www -g www -m 0644 \
	    ${.CURDIR}/homepage.template ${DESTDIR}/var/www/conf/

.include <bsd.prog.mk>

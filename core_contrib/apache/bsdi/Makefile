#	BSDI	Makefile,v 1.4 2002/08/21 23:44:42 polk Exp

NOMAN=noman
NOOBJ=noobj

FILES=	\
	audio.gif		\
	bsd-daemon.gif		\
	bsd-daemon.readme	\
	bsdi-embossed.gif	\
	bsdi-isp.gif		\
	bsdi-logo.gif		\
	bsdi-power.gif		\
	doublewave.gif		\
	powered-by-bsdi.gif	\
	video.gif

PRIVATE= \
	date webmail relay nph-relay

DISABLED= \
	README archie calendar date2 finger fortune nph-test-cgi \
	test-cgi.tcl uptime wais.pl

ENABLED= \
	bsdi-man

beforeinstall:
	-mkdir ${DESTDIR}${WWWDOCSDIR}/bsdi
	-mkdir ${DESTDIR}${WWWPRIVATE}
	-mkdir ${DESTDIR}${WWWCGIDIR}
	-mkdir ${DESTDIR}${WWWCGIBIN}
	(   cd ${.CURDIR}; \
	    install -c -o ${CONFOWN} -g ${CONFGRP} -m ${CONFMODE} \
		${FILES} ${DESTDIR}${WWWDOCSDIR}/bsdi/; \
	    install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${PRIVATE} ${DESTDIR}${WWWPRIVATE}/; \
	    install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${ENABLED} ${DESTDIR}${WWWCGIBIN}/; \
	    install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${DISABLED} ${DESTDIR}${WWWCGIDIR}/; \
	)

.include <bsd.prog.mk>

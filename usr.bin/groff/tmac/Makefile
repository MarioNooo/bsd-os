#	BSDI Makefile,v 2.4 2000/03/08 22:29:22 polk Exp

.include "../Makefile.defs"

MAN7=	ms.0 msafer.0
FILES=  eqnrc tmac.X tmac.Xps tmac.a4 tmac.arkup tmac.dvi tmac.html \
	tmac.latin1 tmac.lj4 tmac.pic tmac.ps tmac.psatk tmac.psnew \
	tmac.psold tmac.pspic tmac.s tmac.safer tmac.trace tmac.tty \
	tmac.tty-char troffrc 
MLINKS= ms.7 groff_ms.7

tags depend:

install: maninstall beforeinstall

beforeinstall:
	for f in ${FILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			${.CURDIR}/$$f ${DESTDIR}${TMACDIR}/; \
	done
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${.CURDIR}/tmac.an ${DESTDIR}${TMACDIR}/tmac.groff_an
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${.CURDIR}/tmac.e ${DESTDIR}${TMACDIR}/tmac.groff_me

.include "../Makefile.common"
.include <bsd.prog.mk>

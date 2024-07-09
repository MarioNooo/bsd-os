#	BSDI	Makefile,v 2.8 1996/10/17 00:07:57 polk Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

.include "skel.en_US/Makefile.inc"

NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	( cd skel.en_US; \
	  for file in ${SKEL_EN_US_FILES}; do \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 0644 $$file \
		${DESTDIR}${BINDIR}/skel/skel.en_US/$$file; \
	  done; \
	  mkdir -p ${DESTDIR}/usr/X11R6/lib/X11/xinit || true; \
	  install -c -o ${BINOWN} -g ${BINGRP} -m 0755 dot.xserverrc \
		${DESTDIR}/usr/X11R6/lib/X11/xinit/xserverrc \
	)

.include <bsd.prog.mk>

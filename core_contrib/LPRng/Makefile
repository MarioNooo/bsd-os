#	BSDI Makefile,v 1.3 2001/06/26 18:56:31 polk Exp

ETC_FILES=	printcap printcap.local
INSTALLCONF=	install -c -m 444

SUBDIR=		LPRng ifhp

afterinstall:
	${INSTALLCONF} ${ETC_FILES} ${DESTDIR}${SYSCONFDIR}

.include <bsd.subdir.mk>

.include "Makefile.inc"

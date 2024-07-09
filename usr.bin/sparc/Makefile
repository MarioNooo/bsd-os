#	BSDI Makefile,v 1.2 1996/10/21 06:47:24 torek Exp

SUBDIR= fdeject

beforeinstall:
	rm -f ${DESTDIR}${BINDIR}/cc; ln -s gcc2 ${DESTDIR}${BINDIR}/cc

.include "${.CURDIR}/../Makefile.inc"
.include <bsd.subdir.mk>

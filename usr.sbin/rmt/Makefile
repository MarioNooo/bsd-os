#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	rmt
MAN8=	rmt.0

afterinstall:
	rm -f ${DESTDIR}/etc/rmt
	ln -s ${BINDIR}/rmt ${DESTDIR}/etc/rmt

.include <bsd.prog.mk>

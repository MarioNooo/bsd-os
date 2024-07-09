# BSDI Makefile,v 1.18 2002/01/28 00:53:48 polk Exp

SUBDIR=	perl lib

afterinstall:
	ln -fs ../perl5/${MACHINE}-bsdos/include \
	    ${DESTDIR}/usr/libdata/perl/include

.include <bsd.subdir.mk>

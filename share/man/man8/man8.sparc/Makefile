#	BSDI	Makefile,v 1.2 1995/10/14 05:13:21 torek Exp
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

MAN8=	MAKEDEV.0 boot.0
MANSUBDIR=/sparc

all: ${MAN8}

clean depend lint tags:

cleandir:
	rm -f ${MAN8}

install: maninstall

.include <bsd.prog.mk>

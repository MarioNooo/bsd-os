#	BSDI	Makefile,v 2.2 1996/09/13 14:05:52 prb Exp
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

MAN8=	MAKEDEV.0 boot.0
MLINKS=	boot.8 boot.default.5
MANSUBDIR=/i386

all: ${MAN8}

clean depend lint tags:

cleandir:
	rm -f ${MAN8}

install: maninstall

.include <bsd.prog.mk>

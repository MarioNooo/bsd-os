#	BSDI	makefile,v 1.1 2000/01/06 22:51:10 polk Exp

all clean check: Makefile
	gmake -f Makefile AM_MAKEFLAGS="-f Makefile" ${.TARGET}

install:
	gmake -f Makefile AM_MAKEFLAGS="-f Makefile" ${.TARGET}
	nroff -mandoc /usr/contrib/man/man1/gpg.1 > /usr/contrib/man/cat1/gpg.0

cleandir: Makefile
	gmake -f Makefile distclean

Makefile: Makefile.in
	./configure --prefix=/usr/contrib --disable-nls


obj depend mansourceinstall objdir:

#	BSDI	Makefile,v 1.6 2003/08/07 21:21:12 polk Exp

all: D/config.status
	(cd D; gmake)

install:
	(cd D; gmake install)
	ln -f /usr/bin/m4 /usr/bin/gm4
	nroff -mandoc /usr/share/core_contrib/man/man1/m4.1 > \
		/usr/share/core_contrib/man/cat1/m4.0

clean cleandir:
	if [ -f D/config.status ]; then \
		(cd D; gmake distclean); \
	fi

depend obj objdir tags maninstall mansourceinstall:

D/config.status:
	(cd D; CC=shlicc CCLD=shlicc ./configure \
		--prefix=/usr \
		--infodir=/usr/share/info \
		--mandir=/usr/share/core_contrib/man \
		--disable-shared \
		--disable-nls \
	)

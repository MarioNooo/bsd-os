CC=	shlicc
BINDIR= /usr/bin
MANDIR=	/usr/share/core_contrib/man
PREFIX=	/usr

all:	Makefile
	touch po/sharutils.pot po/*.gmo
	make -f Makefile all

Makefile:
	touch configure
	CC=${CC} ./configure --prefix=${PREFIX} --disable-uucode \
		--with-gnu-gettext --infodir=/usr/share/info \
		--mandir=/usr/share/core_contrib/man

clean cleandir:
	@if [ -f Makefile ]; then \
		make -f Makefile clean distclean ; \
	fi
	rm -f shar.0 unshar.0

install: Makefile shar.0 unshar.0
	make -f Makefile install
	install -c -o bin -g bin -m 444 shar.0 ${MANDIR}/cat1
	install -c -o bin -g bin -m 444 unshar.0 ${MANDIR}/cat1

shar.0:	doc/shar.1
	groff -mtty-char -Tascii -man doc/shar.1 > shar.0

unshar.0: doc/unshar.1
	groff -mtty-char -Tascii -man doc/unshar.1 > unshar.0

mansourceinstall:	doc/shar.1 doc/unshar.1
	install -c -o bin -g bin -m 444 doc/shar.1 ${MANDIR}/man1
	install -c -o bin -g bin -m 444 doc/unshar.1 ${MANDIR}/man1

obj objdir depend:

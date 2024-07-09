#	BSDI makefile,v 1.7 2003/08/20 21:04:51 polk Exp

all:	Makefile
	make -f Makefile

clean:
	-[ -f Makefile ] && make -f Makefile clean
	rm -f y.tab.c y.tab.h parser-built

cleandir:
	-[ -f Makefile ] && make -f Makefile distclean
	rm -f y.tab.c y.tab.h parser-built

install:
	strip bash
	make -f Makefile install
	groff -mtty-char -Tascii -man doc/readline.3 > \
	    /usr/share/core_contrib/man/cat3/readline.0
	groff -mtty-char -Tascii -man doc/bash.1 > \
	    /usr/share/core_contrib/man/cat1/bash.0

Makefile:
	CC=shlicc ./configure \
		--prefix=/usr/ \
		--bindir=/bin/ \
		--mandir=/usr/share/core_contrib/man/ \
		--infodir=/usr/share/info/

test tests:
	make -f Makefile test

depend:	Makefile

mansourceinstall obj objdir tags:

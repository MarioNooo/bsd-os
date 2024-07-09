#	BSDI	makefile,v 1.2 2000/02/02 21:32:51 polk Exp

all clean: Makefile
	make -f Makefile ${.TARGET}

cleandir: Makefile
	make -f Makefile distclean

Makefile: Makefile.in
	CXXFLAGS=-fno-const-strings \
		bash ./configure --prefix=/usr/contrib --with-gzip=yes \
		--with-x=yes --with-Xpm-library=yes \
		--with-Xpm-includes=/usr/X11/include


install:
	make -f Makefile ${.TARGET}
	@for i in doc/*.1; do \
		echo "installing $$(basename $$i)" ; \
		nroff -mandoc $$i > \
			/usr/contrib/man/cat1/$$(basename $$i .1).0 ; \
	done

obj objdir depend mansourceinstall:

.include <bsd.prog.mk>

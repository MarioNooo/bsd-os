#	BSDI Makefile,v 1.1 2003/02/13 00:58:06 polk Exp

MAN8PAGES=	bgpd.8 ospfd.8 ospf6d.8 ripd.8 ripngd.8 zebra.8

all:	dist/Makefile
	(cd dist; make)

clean:
	-[ -f dist/Makefile ] && (cd dist; make clean)

cleandir:
	-[ -f dist/Makefile ] && (cd dist; make distclean)

install:
	(cd dist; make install)
	sed -e 's#/usr/local#/usr#' -e 's#/usr/etc#/etc#' \
		/usr/share/core_contrib/man/man1/vtysh.1 > \
		/usr/share/core_contrib/man/man1/vtysh.1.new
	mv /usr/share/core_contrib/man/man1/vtysh.1.new \
		/usr/share/core_contrib/man/man1/vtysh.1
	groff -mtty-char -Tascii -mandoc \
		/usr/share/core_contrib/man/man1/vtysh.1 > \
    		/usr/share/core_contrib/man/cat1/vtysh.0
	for i in ${MAN8PAGES}; do \
		sed -e 's#/usr/local#/usr#' -e 's#/usr/etc#/etc#' \
			/usr/share/core_contrib/man/man8/$$i > \
			/usr/share/core_contrib/man/man8/$$i.new ; \
		mv /usr/share/core_contrib/man/man8/$$i.new \
			/usr/share/core_contrib/man/man8/$$i ; \
		groff -mtty-char -Tascii -mandoc \
		  /usr/share/core_contrib/man/man8/$$i > \
	    	  /usr/share/core_contrib/man/cat8/$$(basename $$i .8).0 ; \
	done

dist/Makefile:
	(cd dist; ./configure \
		--prefix=/usr/ \
		--mandir=/usr/share/core_contrib/man/ \
		--sysconfdir=/etc/ \
		--localstatedir=/var/ \
		--infodir=/usr/share/info/ \
	)

depend:	dist/Makefile

mansourceinstall obj objdir tags:

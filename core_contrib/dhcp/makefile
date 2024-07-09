#	BSDI	makefile,v 1.10 2002/09/18 16:33:11 polk Exp

all clean: Makefile
	make -f Makefile ${.TARGET}

install:
	make -f Makefile ${.TARGET}
	install -c -o root -g wheel -m 700 client/scripts/bsdos \
		/etc/dhclient-script
	install -c -o root -g wheel -m 640 client/dhclient.conf \
		/etc/dhclient.conf-sample
	install -c -o root -g wheel -m 640 server/dhcpd.conf \
		/etc/dhcpd.conf-sample
	install -c -o root -g wheel -m 640 /dev/null \
		/var/db/dhcpd.leases
	ln -sf ../etc/dhclient-script /sbin/dhclient-script

cleandir: Makefile
	[ -f work.$$(./configure --print-sysname)/Makefile ] && \
		 make -f Makefile distclean; \
	true

depend: Makefile.conf
	./configure

obj:

mansourceinstall:
	@for i in work*/*/*.man5; do \
		echo "installing $$(basename $$i)" ; \
		install -c -o bin -g bin -m 0444 \
			$$i /usr/share/core_contrib/man/man5/$$(basename $$i .man5).5; \
	done
	@for i in work*/*/*.man8; do \
		echo "installing $$(basename $$i)" ; \
		install -c -o bin -g bin -m 0444 \
			$$i /usr/share/core_contrib/man/man8/$$(basename $$i .man8).8; \
	done

obj objdir:

links:
	make -f Makefile links

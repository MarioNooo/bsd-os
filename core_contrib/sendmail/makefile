#	BSDI	$Id: makefile,v 1.3 1999/09/20 23:24:24 polk Exp $

all:
	make -f Makefile

obj depend tags objdir:

clean cleandir:
	make -f Makefile clean

install:
	make -f Makefile install
	(\
		cd cf; \
		pax -rw -s '#.*CVS.*##' . ${DESTDIR}/usr/share/sendmail; \
	        cd ${DESTDIR}/usr/share/sendmail/cf; \
		m4 < bsdi.mc > ${DESTDIR}/etc/mail/sendmail.cf.dist; \
		chmod 644 ${DESTDIR}/etc/mail/sendmail.cf.dist; \
	)
	if [ ! -f ${DESTDIR}/etc/mail/sendmail.cf ]; then \
		(\
			cp Makefile.mail ${DESTDIR}/etc/mail/Makefile ;\
			cd ${DESTDIR}/etc/mail ;\
			mv sendmail.cf.dist sendmail.cf ;\
			umask 022 ;\
			touch local-host-names ;\
			touch virtual-host-names ;\
			touch mailertable ;\
			touch virtusertable ;\
			touch access ;\
			touch aliases ;\
			$(MAKE) ;\
		); \
	fi
	chown -R root.wheel ${DESTDIR}/usr/share/sendmail
	chmod a+r,go-w ${DESTDIR}/usr/share/sendmail
	(\
		cd doc; \
		pax -rw -s '#.*CVS.*##' . ${DESTDIR}/usr/share/doc/sendmail; \
	)
	chown -R root.wheel ${DESTDIR}/usr/share/doc/sendmail
	chmod a+r,go-w ${DESTDIR}/usr/share/doc/sendmail
	install -c -m 444 -o bin -g bin sendmail/mailq.1 \
		${DESTDIR}/usr/share/core_contrib/man/man1/
	install -c -m 444 -o bin -g bin sendmail/newaliases.1 \
		${DESTDIR}/usr/share/core_contrib/man/man1/
	install -c -m 444 -o bin -g bin sendmail/aliases.5 \
		${DESTDIR}/usr/share/core_contrib/man/man5/
	install -c -m 444 -o bin -g bin sendmail/sendmail.8 \
		${DESTDIR}/usr/share/core_contrib/man/man8/
	install -c -m 444 -o bin -g bin mailstats/mailstats.8 \
		${DESTDIR}/usr/share/core_contrib/man/man8/
	install -c -m 444 -o bin -g bin makemap/makemap.8 \
		${DESTDIR}/usr/share/core_contrib/man/man8/
	install -c -m 444 -o bin -g bin praliases/praliases.8 \
		${DESTDIR}/usr/share/core_contrib/man/man8/
	install -c -m 444 -o bin -g bin rmail/rmail.8 \
		${DESTDIR}/usr/share/core_contrib/man/man8/
	install -c -m 444 -o bin -g bin smrsh/smrsh.8 \
		${DESTDIR}/usr/share/core_contrib/man/man8/

mansourceinstall:

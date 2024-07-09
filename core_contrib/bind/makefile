#	BSDI	$Id$

MANDIR=		/usr/share/core_contrib/man/cat
MANSRCDIR=	/usr/share/core_contrib/man/man

MAN1=	dig.1 host.1

MAN8=	named-checkconf.8 named-checkzone.8 dnssec-keygen.8 \
	dnssec-makekeyset.8 dnssec-signkey.8 dnssec-signzone.8 \
	lwresd.8 named.8 nsupdate.8 rndc-confgen.8 rndc.8

MAN5=	rndc.conf.5

MAN3=	lwres.3 lwres_buffer.3 lwres_config.3 lwres_context.3 lwres_gabn.3 \
	lwres_gai_strerror.3 lwres_getaddrinfo.3 lwres_gethostent.3 \
	lwres_getipnode.3 lwres_getnameinfo.3 lwres_getrrsetbyname.3 \
	lwres_gnba.3 lwres_hstrerror.3 lwres_inetntop.3 lwres_noop.3 \
	lwres_packet.3 lwres_resutil.3

MLINKS= resolver.5 resolv.conf.5

all: Makefile
	make -f Makefile

obj objdir mansourceinstall:

depend: Makefile
	make -f Makefile depend

install:
	make -f Makefile install
	@for i in ${MAN1}; do \
		echo Formatting $$i ; \
		nroff -mandoc ${MANSRCDIR}1/$$i > \
			${MANDIR}1/$$(basename $$i .1).0 ; \
	done
	@for i in ${MAN8}; do \
		echo Formatting $$i ; \
		nroff -mandoc ${MANSRCDIR}8/$$i > \
			${MANDIR}8/$$(basename $$i .8).0 ; \
	done
	@for i in ${MAN5}; do \
		echo Formatting $$i ; \
		nroff -mandoc ${MANSRCDIR}5/$$i > \
			${MANDIR}5/$$(basename $$i .5).0 ; \
	done
	@for i in ${MAN3}; do \
		echo Formatting $$i ; \
		nroff -mandoc ${MANSRCDIR}3/$$i > \
			${MANDIR}3/$$(basename $$i .3).0 ; \
	done

clean:
	if [ -f Makefile ]; then \
	    make -f Makefile clean; \
	fi

cleandir:
	if [ -f Makefile ]; then \
	    make -f Makefile distclean; \
	fi

Makefile:
	./configure --prefix=/usr \
		--sysconfdir=/etc \
		--localstatedir=/var \
		--infodir=/usr/share/info \
		--mandir=/usr/share/core_contrib/man \
		--disable-libbind \
		--enable-ipv6 \
		--with-openssl=/usr \
		--with-kame=/usr \
		--enable-shared \
		--with-libtool \
		--with-randomdev=/dev/random


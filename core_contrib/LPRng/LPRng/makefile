ETC_FILES=		lpd.perms
ROOT_BIN_PROGS=		lpq lprm lpr lpstat
ROOT_SBIN_PROGS=	lpc
SBIN_PROGS=		checkpc lpd lpraccnt
LIBDATA=		UTILS/hello.ps
LIBEXEC_PROGS=		lpf lpbanner
LIBEXEC_SCRIPTS=	smbprint src/pclbanner src/psbanner
INSTALLBIN=	install ${COPY} -o ${BINOWN} -g ${BINGRP}
INSTALLCONF=	install ${COPY} -m 444
LINKS=		${BINDIR}/lpr ${BINDIR}/lp \
		${BINDIR}/lprm ${BINDIR}/cancel
MAKE=	gmake -f Makefile
MAN1!=ls man/*.1 | sed s/.1/.0/
MAN5!=ls man/*.5 | sed s/.5/.0/
MAN8!=ls man/*.8 | sed s/.8/.0/
CLEANFILES+=${MANALL} LPD.CONF
CLEANFILES+=lpd.perms Makefile.bsd intl/po2tbl.sed po/POTFILES
CLEANFILES+=UTILS/LPRng.pm UTILS/lpq_in_perl UTILS/fixid UTILS/lpr_in_perl
CLEANFILES+=UTILS/fixupdate UTILS/read_conf UTILS/accounting.pl
CLEANFILES+=UTILS/lprm_in_perl UTILS/test_read UTILS/remote_active 
CLEANFILES+=UTILS/make_printcap_use

all:	realall manpages LPD.CONF

realall:	Makefile
	${MAKE} all

clean:
	if [ -f Makefile ]; then \
		make -k -f Makefile clean; \
	fi

cleandir:
	if [ -f Makefile ]; then \
		make -k -f Makefile realclean; \
	fi
	rm -rf ${CLEANFILES}

beforeinstall: proginstall docinstall

proginstall:
	cd src && ${INSTALLBIN} ${STRIP} -o root -m 4555 ${ROOT_BIN_PROGS} ${DESTDIR}${BINDIR}
	cd src && ${INSTALLBIN} ${STRIP} -o root -m 4555 ${ROOT_SBIN_PROGS} ${DESTDIR}${SBINDIR}
	cd src && ${INSTALLBIN} ${STRIP} -m ${BINMODE} ${SBIN_PROGS} ${DESTDIR}${SBINDIR}
	cd src && ${INSTALLBIN} ${STRIP} -m ${BINMODE} ${LIBEXEC_PROGS} ${DESTDIR}${LIBEXECDIR}
	${INSTALLBIN} -m ${BINMODE} ${LIBEXEC_SCRIPTS} ${DESTDIR}${LIBEXECDIR}
	${INSTALLCONF} ${ETC_FILES} ${DESTDIR}${SYSCONFDIR}
	${INSTALLCONF} LPD.CONF ${DESTDIR}${SYSCONFDIR}/lpd.conf
	${INSTALLCONF} ${LIBDATA} ${DESTDIR}${LIBDATADIR}

docinstall:
	-rm -rf ${DESTDIR}${DOC_INSTALL_DIR}/LPRng\*
	test -d ${DESTDIR}${DOC_INSTALL_DIR} || mkdir ${DESTDIR}${DOC_INSTALL_DIR}
	cd HOWTO ; \
		find . -name \*.html -o -name \*.gif | \
			pax -rw ${DESTDIR}${DOC_INSTALL_DIR}
	chmod -R 555 ${DESTDIR}${DOC_INSTALL_DIR}
	chown -R bin.bin ${DESTDIR}${DOC_INSTALL_DIR}

Makefile:
	CC="${LDCC}" CFLAGS="${CFLAGS}" PERL=/usr/bin/perl \
		./configure -q \
		${STARG} \
		--prefix=${PREFIX} --sysconfdir=${SYSCONFDIR} \
		--with-filterdir=${LIBEXECDIR}

src man po ./TESTSUPPORT ./UTILS ./HOWTO:	FRC
	${MAKE} -k MAKETARGET="${MAKETARGET}" $@

LPD.CONF:	lpd.conf lpd.conf.suffix
	cat $> > $@

depend:	Makefile

FRC:

obj:

.include <bsd.prog.mk>

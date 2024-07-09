ETC_FILES=	ifhp.conf
PROGS=		ifhp textps
SCRIPTS=	accounting.sh
INSTALL=	install ${COPY} -o ${BINOWN} -g ${BINGRP}
LINKS=		${LIBEXECDIR}/ifhp ${LIBEXECDIR}/ofhp \
		${LIBEXECDIR}/ifhp ${LIBEXECDIR}/banner 
MAN8!=find man -name \*.\[8n] | sed s/\\.\[8n]$$/.0/
MLINKS=		ifhp.8 ofhp.8 \
		ifhp.8 banner.8

MAKE=		gmake -f Makefile

CLEANFILES+=man/*.0

all:	realall manpages
	make ${MANALL}

realall:	Makefile
	 ${MAKE} all

clean:
	if [ -f Makefile ]; then \
		make -k -f Makefile.bsd clean; \
	fi

cleandir:
	if [ -f Makefile ]; then \
		make -k -f Makefile.bsd distclean; \
	fi
	rm -rf ${CLEANFILES}

beforeinstall: proginstall docinstall

proginstall:
	cd src && ${INSTALL} -m ${BINMODE} ${STRIP} ${PROGS} ${DESTDIR}${LIBEXECDIR}
	cd src && ${INSTALL} -m ${BINMODE} ${SCRIPTS} ${DESTDIR}${LIBEXECDIR}
	${INSTALL} -m 444 ${ETC_FILES} ${DESTDIR}${SYSCONFDIR}

docinstall:
	-rm -rf ${DESTDIR}${DOC_INSTALL_DIR}/IFHP\*
	test -d ${DESTDIR}${DOC_INSTALL_DIR} || mkdir ${DESTDIR}${DOC_INSTALL_DIR}
	cd HOWTO ; \
		find . -name \*.html -o -name \*.gif | \
			pax -rw ${DESTDIR}${DOC_INSTALL_DIR}
	chmod -R 555 ${DESTDIR}${DOC_INSTALL_DIR}
	chown -R bin.bin ${DESTDIR}${DOC_INSTALL_DIR}

Makefile:
	A2PS="/usr/contrib/bin/a2ps" GS="/usr/contrib/bin/gs" \
		CC="${LDCC}" CFLAGS="${CFLAGS}" \
		./configure -q \
		--prefix=${PREFIX} --sysconfdir=${SYSCONFDIR} \
		--with-filterdir=${LIBEXECDIR}

depend:	Makefile

src man:	FRC
	${MAKE} MAKETARGET="${MAKETARGET}" $@

FRC:

obj:

.include <bsd.prog.mk>

${MANALLSRC}:
	@cp man/${.TARGET:S/.8$$/.n/} ${.TARGET}


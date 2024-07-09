DOC_INSTALL_DIR=	${DESTDIR}/usr/share/doc/ntp

PROGS= ntpd/ntpd \
	ntpdate/ntpdate \
	ntpdc/ntpdc \
	ntpq/ntpq \
	ntpdate/ntptimeset \
	ntptrace/ntptrace \
	util/tickadj
MAN8=ntp.0
MLINKS+=ntp.8 ntpd.8
MLINKS+=ntp.8 ntpdate.8
MLINKS+=ntp.8 ntpdc.8
MLINKS+=ntp.8 ntpq.8
MLINKS+=ntp.8 ntptimeset.8
MLINKS+=ntp.8 ntptrace.8
MLINKS+=ntp.8 tickadj.8

CLEANFILES+=.warning ntpdate/version.c ntpq/version.c ${MAN8}

all:	realall manpages

realall:	Makefile
	make -f Makefile

clean:
	if [ -f Makefile ]; then \
		make -f Makefile clean; \
	fi
	if [ -n "${CLEANFILES}" ]; then \
		rm -rf ${CLEANFILES}; \
	fi

cleandir:	clean
	if [ -f Makefile ]; then \
		make -f Makefile distclean; \
	fi

realinstall:
	install ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${INSTALLFLAGS} ${PROGS} ${DESTDIR}${BINDIR}

afterinstall:
	-rm -rf ${DOC_INSTALL_DIR} && \
	mkdir ${DOC_INSTALL_DIR} && \
	cd ${.CURDIR}/html && \
		find . \( -name CVS -prune \) -o \( -type f -print \) | \
			pax -rw ${DOC_INSTALL_DIR} && \
	chmod -R 555 ${DOC_INSTALL_DIR} && \
	chown -R bin.bin ${DOC_INSTALL_DIR}

depend:	Makefile

Makefile:
	CC="${LDCC}" CFLAGS="${CFLAGS}" ${.CURDIR}/configure -q \
		--prefix=/usr \
		--enable-step-slew

.include <bsd.prog.mk>

all-recursive all-am distclean-am clean-am:
	make -f Makefile $@

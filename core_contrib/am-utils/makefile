MAN1!=	ls ${.CURDIR}/*/*.1 | sed -e 's/\.1$$/.0/g' -e 's%.*/%%'
MAN5!=	ls ${.CURDIR}/*/*.5 | sed -e 's/\.5$$/.0/g' -e 's%.*/%%'
MAN8!=	ls ${.CURDIR}/*/*.8 | sed -e 's/\.8$$/.0/g' -e 's%.*/%%'
MANDIRS!=	find ${.CURDIR} -name \*.\[0-9\] | sed 's%/[^/]*\.[0-9]$$%%' | sort | uniq
CLEANFILES+=${MANALL}
CLEANFILES+=doc/am-utils.info*
CLEANFILES+=aclocal.m4 a.out aux/aclocal.in

ETCDIR=		/etc
LIBDIR=		${BINDIR:S%/bin%/lib%}
PREFIX=		${BINDIR:S%/bin%%}
INFODIR=	/usr/share/info
DIRFILE=	${INFODIR}/dir
BINDIR=		/usr/bin
SBINDIR=	/usr/sbin

INSTALL=	install ${COPY} -m ${BINMODE} -o ${BINOWN} -g ${BINGRP}
INSTALL_DATA=	install ${COPY} -m 444 -o ${BINOWN} -g ${BINGRP}

PROGS=		amq/pawd 
SCRIPTS= 	scripts/expn scripts/am-eject
SPROGS=		amd/amd \
		amq/amq \
		fixmount/fixmount \
		fsinfo/fsinfo \
		hlfsd/hlfsd \
		mk-amd-map/mk-amd-map \
		wire-test/wire-test
SSCRIPTS=	scripts/amd2ldif scripts/amd2sun \
		scripts/automount2amd \
		scripts/ctl-amd scripts/ctl-hlfsd \
		scripts/fixrmtab scripts/fix-amd-map \
		scripts/lostaltmail scripts/wait4amd \
		scripts/wait4amd2die
ETCS=		${.CURDIR}/amd.conf-sample scripts/lostaltmail.conf-sample
INFO=		doc/am-utils.info*

.PATH: ${MANDIRS}

all:	realall manpages

realall:	Makefile
	touch ${.CURDIR}/aux/aclocal.in
	touch ${.CURDIR}/aux/configure.in
	touch ${.CURDIR}/aux/aclocal.m4
	touch ${.CURDIR}/aux/stamp-h.in
	touch ${.CURDIR}/aux/config.h.in
	touch config.status stamp-h config.h
	${MAKE} -f Makefile

clean:
	if [ -f Makefile ]; then \
		${MAKE} -f Makefile clean; \
	fi
	-rm -f foobar ${CLEANFILES}

cleandir:
	-if [ -f Makefile ]; then \
		${MAKE} -f Makefile distclean; \
	fi
	-rm -f ${CLEANFILES}

afterinstall:
	${INSTALL} libamu/libamu.la ${DESTDIR}${LIBDIR}
	${INSTALL} ${STRIP} ${PROGS} ${DESTDIR}${BINDIR}
	${INSTALL} ${SCRIPTS} ${DESTDIR}${BINDIR}
	${INSTALL} ${STRIP} ${SPROGS} ${DESTDIR}${SBINDIR}
	${INSTALL} ${SSCRIPTS} ${DESTDIR}${SBINDIR}
	(cd ${.CURDIR}; ${INSTALL_DATA} ${ETCS} ${DESTDIR}${ETCDIR})
	(cd ${.CURDIR}; ${INSTALL_DATA} ${INFO} ${DESTDIR}${INFODIR} ; \
	install-info --dir-file=${DIRFILE} --quiet --section='System tools' \
		--item='* Am-utils: (am-utils).          The Amd automounter suite of utilities' \
                doc/am-utils.info)

depend:	Makefile

obj:

Makefile:
	ln -sf auy aux
	CC="${LDCC}" CFLAGS="${CFLAGS}" \
		${.CURDIR}/configure -q \
		--prefix=${PREFIX} \
		--sysconfdir=${ETCDIR} --includedir=/usr/include

.include <bsd.prog.mk>

all-am all-recursive clean-am clean-recursive distclean-am distclean-recursive:
	@${MAKE} -f Makefile ${.TARGET}

BINDIR=		/usr/bin
MANDIR=		/usr/share/core_contrib/man/cat
MANSRCDIR=	/usr/share/core_contrib/man/man
DATADIR=	${DESTDIR}/usr/share/vi
INSTALL=	install ${COPY} -o ${BINOWN} -g ${BINGRP}
MAN1=		vi.0
MLINKS+=	vi.1 ex.1 \
		vi.1 nex.1 \
		vi.1 nvi.1 \
		vi.1 view.1
PROG=		nvi
LINKS=		${BINDIR}/nvi ${BINDIR}/ex \
		${BINDIR}/nvi ${BINDIR}/nex \
		${BINDIR}/nvi ${BINDIR}/vi \
		${BINDIR}/nvi ${BINDIR}/view
CATALOGS!=	ls ${.CURDIR}/catalog/*.base | sed 's/.base$$//'
PERLS!=		ls ${.CURDIR}/perl_scripts
TCLS!=		ls ${.CURDIR}/tcl_scripts
CLEANFILES+=	VI.pm vi.1 vi.0

all:	realall manpages

realall:	Makefile
	${MAKE} -f Makefile

afterinstall:
	install -c -m 444 -o bin -g bin ${CATALOGS} ${DATADIR}/catalog
	if grep -Eq '^#define[:blank:]+HAVE_PERL_INTERP' config.h; then \
		${INSTALL} -m 444 ${PERLS} ${DATADIR}/perl; \
	fi
	if grep -Eq '^#define[:blank:]+HAVE_TCL_INTERP' config.h; then \
		${INSTALL} -m 444 ${TCLS} ${DATADIR}/tcl; \
	fi
	${INSTALL} -m ${BINMODE} recover ${DATADIR}

clean:
	if [ -f Makefile ]; then \
		${MAKE} -f Makefile clean; \
	fi
	rm -f ${CLEANFILES}

cleandir:
	if [ -f Makefile ]; then \
		${MAKE} -f Makefile distclean; \
	fi
	rm -f ${CLEANFILES}

depend:	Makefile

Makefile:
	${.CURDIR}/build/configure -q --prefix=/usr \
		--disable-curses --disable-db --disable-re

vi.1:	${.CURDIR}/docs/USD.doc/vi.man/vi.1
	ln -s $> $@

.include <bsd.prog.mk>

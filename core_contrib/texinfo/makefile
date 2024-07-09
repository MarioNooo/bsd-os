#	BSDI makefile,v 1.4 2003/08/07 17:42:44 polk Exp

PREFIX=		\$${DESTDIR}/usr
INSTALL=	/usr/bin/install ${COPY} -o ${BINOWN} -g ${BINGRP}
PROGS=		info/ginfo makeinfo/makeinfo \
		util/texindex util/texi2dvi util/ginstall-info
INFOS=		doc/*.info doc/texinfo doc/texinfo-*
INFODIR=	${DESTDIR}/usr/share/info
MAN1!=ls D/*/*.1 | sed -e /stamp-vti/d -e s/.1/.0/
MAKE=make

all:	${.CURDIR}/D/Makefile realall manpages

realall:
	cd ${.CURDIR}/D; \
	${MAKE} -f Makefile all

clean:
	cd ${.CURDIR}/D; \
	if [ -f Makefile ]; then \
		${MAKE} -f Makefile clean; \
	fi

cleandir:
	cd ${.CURDIR}/D; \
	if [ -f Makefile ]; then \
		${MAKE} -f Makefile distclean; \
	fi
	rm -f ${MANALL}

${.CURDIR}/D/Makefile:
	cd ${.CURDIR}/D; \
	LIBDIR=${PREFIX}/share CC="${LDCC}" CFLAGS="${CFLAGS}" \
		./configure -q --prefix=${PREFIX} \
			--infodir=${PREFIX}/share/info \
			--mandir=${PREFIX}/share/core_contrib/man \

XINSTALL=	${INSTALL} ${COPY} -m ${BINMODE} -o ${BINOWN} -g ${BINGRP}

realinstall:
	cd ${.CURDIR}/D; \
	${XINSTALL} ${STRIP} info/ginfo ${DESTDIR}${BINDIR}/info; \
	${XINSTALL} ${STRIP} makeinfo/makeinfo ${DESTDIR}${BINDIR}/makeinfo; \
	${XINSTALL} ${STRIP} util/texindex ${DESTDIR}${BINDIR}/texindex; \
	${XINSTALL} util/texi2dvi ${DESTDIR}${BINDIR}/texi2dvi; \
	${XINSTALL} ${STRIP} util/ginstall-info ${DESTDIR}${BINDIR}/install-info; \
	[ -f ${INFODIR}/dir ] || \
		${MINSTALL} ${.CURDIR}/../dir ${INFODIR}/dir; \
	for file in ${INFOS}; do \
		f=$$(basename $$file); \
		${MINSTALL} $$file ${INFODIR}; \
		./util/ginstall-info --info-dir=${INFODIR} ${INFODIR}/$$f || true; \
	done

depend: ${.CURDIR}/D/Makefile

obj objdir tags:

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/D

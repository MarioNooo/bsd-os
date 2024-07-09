#	BSDI	makefile,v 1.8 2003/08/28 20:57:40 polk Exp

DATADIR=	/usr/share
LIBDIR=		/usr/lib
BINDIR=		/usr/bin
MANDIR=		/usr/share/core_contrib/man/cat
MANSRCDIR=	/usr/share/core_contrib/man/man
SHLIBDIR=	/shlib
LIBGRP=		bin
LIBOWN=		bin
LIBMODE=	444
INSTALL=	/usr/bin/install ${COPY} -o ${BINOWN} -g ${BINGRP}
INSTALL_DATA=	${INSTALL} -m 444
INSTALL_PROG=	${INSTALL} ${STRIP} -m ${BINMODE}

MANLINKS= \
	${MANDIR}1/tset.0 ${MANDIR}1/reset.0 \
	${MANDIR}3/curs_termcap.0 ${MANDIR}3/termcap.0 \
	${MANDIR}3/curs_termcap.0 ${MANDIR}3/tgetent.0 \
	${MANDIR}3/curs_termcap.0 ${MANDIR}3/tgetflag.0 \
	${MANDIR}3/curs_termcap.0 ${MANDIR}3/tgetnum.0 \
	${MANDIR}3/curs_termcap.0 ${MANDIR}3/tgetstr.0 \
	${MANDIR}3/curs_termcap.0 ${MANDIR}3/tgoto.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/mvcur.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/putp.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/restartterm.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/set_curterm.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/setterm.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/setuptterm.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/terminfo.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/tigetflag.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/tigetnum.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/tigetstr.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/tparm.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/tputs.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/vidattr.0 \
	${MANDIR}3/curs_terminfo.0 ${MANDIR}3/vidputs.0 \
	${MANDIR}3/ncurses.0 ${MANDIR}3/curses.0 \
	${MANDIR}5/terminfo.0 ${MANDIR}5/termcap.0

# XXX links for termcap, curses, termlib
LIBLINKS= \
	${LIBDIR}/libncurses.a ${LIBDIR}/libcurses.a \
	${LIBDIR}/libncurses.a ${LIBDIR}/libtermlib.a \
	${LIBDIR}/libncurses.a ${LIBDIR}/libtermcap.a

SHLIBLINKS= \
	${SHLIBDIR}/libncurses.so${LDDYNSUF} ${SHLIBDIR}/libcurses.so \
	${SHLIBDIR}/libncurses.so${LDDYNSUF} ${SHLIBDIR}/libtermlib.so \
	${SHLIBDIR}/libncurses.so${LDDYNSUF} ${SHLIBDIR}/libtermcap.so

all:	Makefile
	make -f Makefile

Makefile:
	./configure -q \
		--prefix=/usr \
		--datadir=${DATADIR} \
		--includedir=/usr/include \
		--libdir=${LIBDIR} \
		--enable-bsdpad --enable-const --enable-termcap \
		--enable-getcap --enable-broken-linker --without-debug \
		--with-profile --with-libtool

clean:
	if [ -f Makefile ]; then \
		make -f Makefile clean; \
	fi

cleandir:
	if [ -f Makefile ]; then \
		make -f Makefile distclean; \
	fi

install: libinstall maninstall
	make -f Makefile DESTDIR=${DESTDIR} install.progs
	make -f Makefile DESTDIR=${DESTDIR} install.data
	for file in $$(find ${DESTDIR}${DATADIR}/terminfo \
		${DESTDIR}${DATADIR}/tabset -type f); do \
		chown ${BINOWN}.${BINGRP} $${file}; \
		chmod 444 $${file}; \
	done

libinstall:
	make -f Makefile DESTDIR=${DESTDIR} install.includes
	make -f Makefile DESTDIR=${DESTDIR} install.libs
	mv ${DESTDIR}/usr/lib/libncurses.so* ${DESTDIR}/shlib
	ln -s /shlib/libncurses.so ${DESTDIR}/usr/lib
	mv ${DESTDIR}/usr/lib/libncurses++.so* ${DESTDIR}/shlib
	ln -s /shlib/libncurses++.so ${DESTDIR}/usr/lib
	ldconfig
	${INSTALL_DATA} libncurses.except ${DESTDIR}/usr/lib/
	mkshlib -lcurses
	@set ${LIBLINKS}; \
	while test $$# -ge 2; do \
		l=${DESTDIR}$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		echo $$t -\> $$l; \
		rm -f $$t; \
		ln $$l $$t; \
	done; true
	@set ${SHLIBLINKS}; \
	while test $$# -ge 2; do \
		l=${DESTDIR}$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		echo $$t -\> $$l; \
		rm -f $$t; \
		ln -s $$l $$t; \
	done; true

maninstall:
	@(cd man; make terminfo.5)
	@(for i in man/*.[1-9]* tack/tack.1; do \
	    base=$$(basename $$i | sed -e 's/\.[1-9].*//') ; \
	    sect=$$(basename $$i | sed -e 's/.*\.\([0-9]\).*/\1/') ; \
	    sed	-e "s,@DATADIR@,${DATADIR}," \
		-e "s,@TERMINFO@,/usr/share/terminfo," \
		-e "s,@NCURSES_OSPEED@,short," \
		-e "s,@CAPTOINFO@,captoinfo," \
		-e "s,@CLEAR@,clear," \
		-e "s,@INFOCMP@,infocmp," \
		-e "s,@INFOTOCAP@,infotocap," \
		-e "s,@TIC@,tic," \
		-e "s,@TOE@,toe," \
		-e "s,@TPUT@,tput," \
                < $$i > $$i.tmp ; \
	    echo "Installing mansrc page $$base.$$sect" ; \
	    ${INSTALL_DATA} $$i.tmp \
		${DESTDIR}${MANSRCDIR}$$sect/$$base.$$sect ; \
	    echo "Installing formatted man page $$base.$$sect" ; \
	    tbl $$i.tmp 2>/dev/null | nroff -mandoc > $$i.0 ; \
	    ${INSTALL_DATA} $$i.0 \
		${DESTDIR}${MANDIR}$$sect/$$base.0 ; \
	    rm $$i.tmp ; \
	    rm $$i.0 ; \
	done)
	@set ${MANLINKS}; \
	while test $$# -ge 2; do \
		l=${DESTDIR}$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		echo $$t -\> $$l; \
		rm -f $$t; \
		ln $$l $$t; \
	done; true

libs:	Makefile
	make -f Makefile libs

# Configure expects this rule to exist in makefile
preinstall:
	make -f Makefile preinstall

depend:	Makefile

obj objdir mansourceinstall:

.include <bsd.prog.mk>

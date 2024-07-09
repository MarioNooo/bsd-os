#	BSDI makefile,v 1.5 2003/09/05 00:09:29 polk Exp

BINDIR=		/usr/bin
INSTALL?=	install

# probably should use "obj", but plain make likes to chdir into
# obj subdirectories, so for now...
COMPILEDIR=	compile

GMAKE=		gmake

# BINUTILS just lists the stuff we install from the binutils subdirectory.
# We could perhps list them as binutils/foo and handle gprof/gprof here
# too, but "gas" and "ld" need special installation lines anyway.
#
# Note that ranlib is a link to ar (forced in installation below),
# but the man pages are separate (also forced below).
BINUTILS=	ar objcopy objdump

all: compile/Makefile
	(cd compile; ${GMAKE})

clean:
	if [ -f compile/Makefile ]; then \
	    (cd compile; ${GMAKE} clean); \
	fi

cleandir:
	rm -rf compile

install: realinstall maninstall

# We install the binaries "manually" so that we control where
# they land and which ones get installed.
realinstall:
	(cd ${COMPILEDIR}/binutils; \
	for i in ${BINUTILS}; do \
	    ${INSTALL} ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} \
	    -m ${BINMODE} ${INSTALLFLAGS} $$i ${DESTDIR}${BINDIR}/$$i; \
	done)
	rm -f ${DESTDIR}${BINDIR}/ranlib; \
	    ln ${DESTDIR}${BINDIR}/ar ${DESTDIR}${BINDIR}/ranlib
	(cd ${COMPILEDIR}/gas; \
	    ${INSTALL} ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} \
	    -m ${BINMODE} ${INSTALLFLAGS} as-new ${DESTDIR}${BINDIR}/as)
	(cd ${COMPILEDIR}/gprof; \
	    ${INSTALL} ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} \
	    -m ${BINMODE} ${INSTALLFLAGS} gprof ${DESTDIR}${BINDIR}/gprof)
	(cd ${COMPILEDIR}/ld; \
	    ${INSTALL} ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} \
	    -m ${BINMODE} ${INSTALLFLAGS} ld-new ${DESTDIR}${BINDIR}/ld)

# We have to install man pages "manually", as it were.
# The patch stuff is a bit of an experiment.
# The info files, and "po" message-catalog files, we install with
# the GNU makefiles.  For tradition's sake we put the info files
# in /usr/share/info instead of /usr/contrib/info, though.
.if exists(${DESTDIR}${MANSRCDIR}1${MANSUBDIR})
MSRCINSTALL=${MINSTALL}
.else
MSRCINSTALL=echo not installing:
.endif
maninstall:
	(T=/tmp/doc$$$$; rm -rf $$T; trap 'rm -rf $$T' 0 1 2 3 15; mkdir $$T; \
	d=${.CURDIR}/binutils/doc; \
	for i in ${BINUTILS:S/$/.1/g} ranlib.1; do \
	    cp $$d/$$i $$T/$$i; \
	    if [ -f ${.CURDIR}/patch/$$i ]; then \
		(cd $$T; patch $$i < ${.CURDIR}/patch/$$i); \
	    fi; \
	    o=`echo $$i | sed 's/.1$$/.0/'`; \
	    (cd $$T; nroff -man $$i > $$o; \
	        ${MINSTALL} $$o ${DESTDIR}${MANDIR}1${MANSUBDIR}/$$o; \
		${MSRCINSTALL} $$i ${DESTDIR}${MANSRCDIR}1${MANSUBDIR}/$$i); \
	done; \
	d=${.CURDIR}/gas/doc; i=as.1; o=as.0; \
	(cd $$T; nroff -man $$d/$$i > $$o; \
	    ${MINSTALL} $$o ${DESTDIR}${MANDIR}1${MANSUBDIR}/$$o; \
	    ${MSRCINSTALL} $$d/$$i ${DESTDIR}${MANSRCDIR}1${MANSUBDIR}/$$i); \
	d=${.CURDIR}/gprof; i=gprof.1; o=gprof.0; \
	(cd $$T; nroff -man $$d/$$i > $$o; \
	    ${MINSTALL} $$o ${DESTDIR}${MANDIR}1${MANSUBDIR}/$$o; \
	    ${MSRCINSTALL} $$d/$$i ${DESTDIR}${MANSRCDIR}1${MANSUBDIR}/$$i); \
	d=${.CURDIR}/ld; i=ld.1; o=ld.0; \
	(cd $$T; nroff -man $$d/$$i > $$o; \
	    ${MINSTALL} $$o ${DESTDIR}${MANDIR}1${MANSUBDIR}/$$o; \
	    ${MSRCINSTALL} $$d/$$i ${DESTDIR}${MANSRCDIR}1${MANSUBDIR}/$$i))
	for i in binutils/doc gas/doc gprof ld; do \
	    (cd ${COMPILEDIR}/$$i; \
	     ${GMAKE} prefix=/usr/share install-info-am); \
	done
	for i in binutils gas gprof ld; do \
	    (cd ${COMPILEDIR}/$$i/po; ${GMAKE} prefix=/usr install); \
	done

depend obj objdir:
	@-[ -d ${COMPILEDIR} ] || mkdir ${COMPILEDIR}
	@-[ -f ${COMPILEDIR}/Makefile ] || \
	    (cd ${COMPILEDIR}; sh ${.CURDIR}/configure \
		--prefix=/usr/contrib \
	)

tags:

.include <bsd.prog.mk>

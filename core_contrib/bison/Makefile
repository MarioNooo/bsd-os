DESTDIR=		/usr
PREFIX=                 ${DESTDIR}
PROGRAMPREFIX=		${DESTDIR}
BINDIR=                 ${DESTDIR}/bin
INSTALL=                /usr/bin/install -o bin -g bin -c
INSTALL_PROGRAM=        ${INSTALL} -m 555
INSTALL_DATA=           ${INSTALL} -m 444
MAN1SRC=                ${DESTDIR}/share/core_contrib/man/man1
MAN1DIR=                ${DESTDIR}/share/core_contrib/man/cat1
 
all:    D/Makefile
	cd D;make -f Makefile
 
clean:
	cd D;test -f Makefile && make -f Makefile clean
	cd D;rm -f Makefile
	cd D;rm -f *.0

cleandir: clean
	cd D;rm -f config.cache config.h config.log config.status
 
D/Makefile:
	cd D;CC='${CC}' INSTALL='${INSTALL}' INSTALL_PROGRAM='${INSTALL_PROGRAM}' \
	    INSTALL_DATA='${INSTALL_DATA}' \
	    ./configure --prefix=${PREFIX} \
	    --exec-prefix=${PREFIX} \
	    --mandir=${PREFIX}/share/core_contrib/man \
	    --infodir=${PREFIX}/share/info \
	    --bindir=${BINDIR}

DIRFILE=${DESTDIR}/info

install:
	cd D;make -f Makefile install
	cd D/doc;groff -mtty-char -Tascii -man bison.1 > bison.0 && \
	${INSTALL_DATA} $$man.0 ${MAN1DIR}; 

obj depend maninstall mansourceinstall:


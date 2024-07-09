PREFIX=			/usr
LIBDIR=			${PREFIX}/lib
INSTALL_PROGRAM=	/usr/bin/install -m 555 -o bin -g bin -c -s
INSTALL_DATA=		/usr/bin/install -m 444 -o bin -g bin
MAN1PAGES=		less lesskey
MAN1DIR=		/usr/share/core_contrib/man/cat1

all:	Makefile
	make -f Makefile CC=shlicc datadir=${LIBDIR}

clean:
	-test -f Makefile && make -f Makefile clean

cleandir:	clean
	rm -f Makefile config.status config.log config.cache defines.h stamp-h

Makefile:	
	./configure --prefix=${PREFIX} \
		--mandir=${PREFIX}/share/core_contrib/man \
		--datadir=${PREFIX}/share --sysconfdir=/etc

install:
	make -f Makefile \
		prefix=${PREFIX} datadir=${LIBDIR} \
		INSTALL_PROGRAM='${INSTALL_PROGRAM}' \
		INSTALL_DATA='${INSTALL_DATA} -c' \
		install
	make maninstall

maninstall:
	for man in ${MAN1PAGES}; do \
		groff -mtty-char -Tascii -man $$man.nro > $$man.0 && \
			${INSTALL_DATA} $$man.0 ${MAN1DIR}; \
	done

depend:	Makefile

mansourceinstall obj objdir tags:

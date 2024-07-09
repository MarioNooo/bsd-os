#PROG=racoon
MAN8=racoon.0
MAN5=racoon.conf.0
OWN=-o ${BINOWN} -g ${BINGRP}

ETC=/etc

CONF=racoon.conf psk.txt
SAMPLES=Makefile psk.txt.sample \
	racoon.conf.sample racoon.conf.sample-gssapi \
	sandiego.pl

all: Makefile ${MAN8} ${MAN5}
	make -f Makefile ${.TARGET}

depend: Makefile
	make -f Makefile ${.TARGET}

Makefile: Makefile.in configure /usr/lib/libipsec.a
	./configure --prefix=/usr --sysconfdir=${ETC} \
		--with-libpfkey \
		--mandir=/usr/share/man --enable-debug

racoon kmpstat pfkey eaytest: Makefile forceit
	make -f Makefile ${.TARGET}

forceit:

clean: Makefile
	make -f Makefile ${.TARGET}

cleandir: Makefile
	make -f Makefile distclean
	rm -f .depend ${MANALL}


# We don't define PROG, so we install "racoon" here.
afterinstall:
	install ${COPY} ${STRIP} ${OWN} -m ${BINMODE} \
	    ${INSTALLFLAGS} racoon ${DESTDIR}${BINDIR}
	-mkdir -p ${ETC}/racoon
	for i in ${CONF}; do \
	    install -c ${OWN} -m 444 samples/$$i ${ETC}/racoon/$$i.dist; \
	done
	for i in ${SAMPLES}; do \
	    install -c ${OWN} -m 444 samples/$$i ${ETC}/racoon/$$i; \
	done

obj objdir depend:

.include "bsd.prog.mk"

PREFIX=			\$${DESTDIR}/usr
INSTALL=		install -o ${BINOWN} -g ${BINGRP} ${COPY}
INSTALL_PROGRAM=	${INSTALL} -m ${BINMODE} 
INSTALL_DATA=		${INSTALL} -m 444
MAN1PAGES=		gawk igawk
PROGS=	gawk

all:	Makefile
	make -f Makefile

clean:
	if [ -f Makefile ]; then \
		make -f Makefile ${MAKEFLAGS} clean ; \
	fi

cleandir: clean
	if [ -f Makefile ]; then \
		make -f Makefile ${MAKEFLAGS} distclean ; \
	fi

Makefile:	
	CC="${LDCC}" CFLAGS="${CFLAGS}" \
		INSTALL='${INSTALL}' INSTALL_PROGRAM='${INSTALL_PROGRAM}' \
		INSTALL_DATA='${INSTALL_DATA}' \
		./configure --prefix=${PREFIX} --exec-prefix=${PREFIX} \
			--infodir=${PREFIX}/share/info \
			--mandir=${MANSRCDIR}

install:
	install ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${INSTALLFLAGS} ${PROGS} ${DESTDIR}${BINDIR}
	-cd ${DESTDIR}${BINDIR} && \
		test -f gawk && rm -f awk && ln gawk awk
	cd doc && make ${MAKEFLAGS} install
	cd awklib && make ${MAKEFLAGS} install

depend:	Makefile

mansourceinstall:
	cd doc && make ${MAKEFLAGS} mansourceinstall

obj objdir tags:

.include <bsd.prog.mk>

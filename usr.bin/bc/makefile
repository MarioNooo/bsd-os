PREFIX=			\$${DESTDIR}/usr
INSTALL=		install -o ${BINOWN} -g ${BINGRP} ${COPY}
INSTALL_PROGRAM=	${INSTALL} -m ${BINMODE} ${STRIP}
INSTALL_DATA=		${INSTALL} -m 444
PROGS=	bc/bc dc/dc

all:	Makefile
	make -f Makefile ${MAKEFLAGS} all-recursive

clean:
	if [ -f Makefile ]; then \
		make -f Makefile ${MAKEFLAGS} clean ; \
	fi

cleandir: clean
	if [ -f Makefile ]; then \
		make -f Makefile ${MAKEFLAGS} distclean ; \
	fi
	rm -f bc/bc.c bc/bc.h

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
	cd doc && make ${MAKEFLAGS} install

depend:	Makefile

mansourceinstall:
	cd doc && make ${MAKEFLAGS} mansourceinstall

obj objdir tags:

.include <bsd.prog.mk>

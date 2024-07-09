INSTALL=	install ${COPY} -m ${BINMODE} -o ${BINOWN} -g ${BINGRP}
INSTALL_PROGRAM=	${INSTALL} ${STRIP}
INSTALL_DATA=	${INSTALL} -m 444
BINDIR=		/usr/bin
LIBDIR=		/usr/share/misc
MANDIR=		/usr/share/core_contrib/man/cat
MANSRCDIR=	/usr/share/core_contrib/man/man
MAN1=		file.0
MAN4=		magic.0
DEFS=		-DBUILTIN_ELF

all:	realall manpages

realall:	Makefile
	make -f Makefile

clean:
	if [ -f Makefile ]; then \
		make -f Makefile clean; \
	fi
	rm -f ${MAN1} ${MAN4}

cleandir: clean
	rm -f Makefile

install: realinstall maninstall

realinstall:
	${INSTALL} file ${BINDIR}
	${INSTALL_DATA} magic ${LIBDIR}
	${INSTALL_DATA} magic.mime ${LIBDIR}
	${INSTALL_DATA} magic.mgc ${LIBDIR}

Makefile:
	CC="${LDCC}" CFLAGS="${CFLAGS} ${DEFS}" \
		 ./configure -q --prefix=/usr  --datadir=/usr/share/misc

depend obj objdir tags:

.include <bsd.prog.mk>

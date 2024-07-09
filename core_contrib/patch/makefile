MAN1=patch.0
PROG=patch

all:	Makefile manpages
	make -f Makefile ${MAKEFLAGS} ${PROG}

clean:
	if [ -f Makefile ]; then \
		make -f Makefile ${MAKEFLAGS} clean; \
	fi
	rm -f ${MANALL}

cleandir:
	if [ -f Makefile ]; then \
		make -f Makefile ${MAKEFLAGS} distclean; \
	fi
	rm -f patch.1

depend:	Makefile

Makefile:
	touch configure
	CC="${LDCC}" CFLAGS="${CFLAGS}" \
		./configure --prefix=${BINDIR:S./bin..}

patch.1:	patch.man
	ln -s $> $@

obj objdir tags:

.include <bsd.prog.mk>

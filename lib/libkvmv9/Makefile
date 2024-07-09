#	BSDI Makefile,v 1.1 1998/08/27 15:30:27 torek Exp
#
#	@(#)Makefile	8.3 (Berkeley) 5/4/95

LIB=	kvmv9
CFLAGS+=-DSPARCV9
MACHINE=sparc_v9

.PATH: ${.CURDIR}/../libkvm

# oddly enough, this works -- why?
beforeinstall:
	-cd ${.CURDIR}/../libkvm; cmp -s kvm_stat.h \
	    ${DESTDIR}/usr/include/kvm_stat.h >/dev/null 2>&1 || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 kvm_stat.h \
	    ${DESTDIR}/usr/include

.include "../libkvm/Makefile"

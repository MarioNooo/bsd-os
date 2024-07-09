#	BSDI Makefile,v 1.1 1998/08/27 15:30:26 torek Exp
#
#	@(#)Makefile	8.3 (Berkeley) 5/4/95

LIB=	kvmv8

.PATH: ${.CURDIR}/../libkvm
MACHINE=sparc

# Have to duplicate this rule from ../libkvm/Makefile, modified to
# refer to ../libkvm.
beforeinstall:
	-cd ${.CURDIR}/../libkvm; cmp -s kvm_stat.h \
	    ${DESTDIR}/usr/include/kvm_stat.h >/dev/null 2>&1 || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 kvm_stat.h \
	    ${DESTDIR}/usr/include

.include "../libkvm/Makefile"

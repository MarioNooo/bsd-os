#	BSDI Makefile,v 2.13 2002/01/18 21:06:33 polk Exp
#
#	@(#)Makefile	8.3 (Berkeley) 5/4/95

# N.B.: libkvmv8 and libkvmv9 include this file after setting LIB
LIB?=	kvm

# For compatible changes, increment minor
# For incompatible changes use increment major
LDDYNMAJOR=.1
LDDYNMINOR=.0

CFLAGS+=-DLIBC_SCCS -I/sys
LDDYNDEP=-lc
SRCS=	kvm.c kvm_${MACHINE}.c kvm_boottime.c kvm_cpustats.c kvm_disks.c \
	kvm_file.c kvm_getloadavg.c kvm_iflist.c kvm_kmem.c kvm_ncache.c \
	kvm_proc.c kvm_stathz.c kvm_ttys.c kvm_ttytotals.c kvm_util.c \
	kvm_vmmeter.c kvm_vmtotal.c kvm_vnodes.c

MAN3=	kvm.0 kvm_boottime.0 kvm_cpustats.0 kvm_disks.0 kvm_files.0 \
	kvm_geterr.0 kvm_getloadavg.0 kvm_getprocs.0 kvm_iflist.0 kvm_kmem.0 \
	kvm_ncache.0 kvm_nlist.0 kvm_open.0 kvm_read.0 kvm_ttys.0 \
	kvm_vmmeter.0 kvm_vnodes.0

MLINKS+=kvm_getprocs.3 kvm_getargv.3 kvm_getprocs.3 kvm_getenvv.3
MLINKS+=kvm_open.3 kvm_openfiles.3 kvm_open.3 kvm_close.3
MLINKS+=kvm_read.3 kvm_write.3
MLINKS+=kvm_disks.3 kvm_dknames.3
MLINKS+=kvm_cpustats.3 kvm_stathz.3
MLINKS+=kvm_kmem.3 kvm_kmemsize.3
MLINKS+=kvm_vmmeter.3 kvm_vmtotal.3
MLINKS+=kvm_ttys.3 kvm_ttytotals.3
MLINKS+=kvm_iflist.3 kvm_ifmulti.3

beforeinstall:
	-cd ${.CURDIR}; cmp -s kvm_stat.h \
	    ${DESTDIR}/usr/include/kvm_stat.h >/dev/null 2>&1 || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 kvm_stat.h \
	    ${DESTDIR}/usr/include

.include <bsd.lib.mk>

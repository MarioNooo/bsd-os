#	BSDI Makefile,v 2.25 2002/09/03 16:56:01 geertj Exp
#	@(#)Makefile	8.2 (Berkeley) 4/16/94

.if ${MACHINE} == "sparc_v9"
MCH=	sparc
.else
MCH=	${MACHINE}
.endif

# No Fortran support yet (man3f).
SUBDIR+=man1 man3 man5 man6 man7 man8
SUBDIR+=../../sys/dev
SUBDIR+=../../sys/dev/evtlog
SUBDIR+=../../sys/dev/pci
SUBDIR+=../../sys/dev/scsi
SUBDIR+=../../sys/dev/mii
SUBDIR+=../../sys/dev/serial/ns16550
SUBDIR+=../../sys/dev/serial/mpc8260smc
SUBDIR+=../../sys/dev/netif/ether/i8255x
SUBDIR+=../../sys/dev/netif/ether/mpc8260fcc
.if exists(../../sys/bsp/wr-ppmc7xx/man)
SUBDIR+=../../sys/bsp/wr-ppmc7xx/man
.endif
.if exists(../../sys/bsp/wr-sbc8260/man)
SUBDIR+=../../sys/bsp/wr-sbc8260/man
.endif
.if exists(../../sys/i386)
SUBDIR+=../../sys/i386/eisa
SUBDIR+=../../sys/i386/i386
SUBDIR+=../../sys/i386/isa
SUBDIR+=../../sys/i386/pci
.endif
.if exists(../../sys/i386/bc)
SUBDIR+=../../sys/i386/pci/bc
.endif
.if exists(../../sys/i386/em)
SUBDIR+=../../sys/i386/pci/em
.endif
.if exists(../../sys/i386/objsrc)
SUBDIR+=../../sys/i386/objsrc
.endif
SUBDIR+=../../sys/isofs/cd9660
SUBDIR+=../../sys/miscfs/fifofs
SUBDIR+=../../sys/kern
SUBDIR+=../../sys/net
.if ${MCH} == "sparc"
SUBDIR+=../../sys/netatm
SUBDIR+=../../sys/dev/atm/fore
.endif
SUBDIR+=../../sys/netinet
.if exists(../../sys/sparc)
SUBDIR+=../../sys/sparc/dev
SUBDIR+=../../sys/sparc/sbus
SUBDIR+=../../sys/sparc/sparc
.endif
.if exists(../../sys/powerpc)
SUBDIR+=../../sys/powerpc/powerpc
.endif

makedb:
	find /usr/share/man -type f -name '*.0' -print | \
	    /usr/share/man/makewhatis.perl | \
	    sort -u > ${DESTDIR}/usr/share/man/whatis.db
	chown ${BINOWN}.${BINGRP} ${DESTDIR}/usr/share/man/whatis.db
	chmod 444 ${DESTDIR}/usr/share/man/whatis.db

.include <bsd.subdir.mk>

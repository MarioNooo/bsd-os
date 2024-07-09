#	BSDI	Makefile,v 2.4 1998/09/22 13:06:08 torek Exp
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.if ${MACHINE} == "sparc_v9"
MCH=	sparc
.else
MCH=	${MACHINE}
.endif

PROG=	reboot
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN8=	reboot.0 
MAN8+=	boot_${MCH}.0
MLINKS=	reboot.8 halt.8 reboot.8 fastboot.8 reboot.8 fasthalt.8
LINKS=	${BINDIR}/reboot ${BINDIR}/halt ${BINDIR}/reboot ${BINDIR}/fastboot \
	${BINDIR}/reboot ${BINDIR}/fasthalt

#afterinstall:
#	${MINSTALL} boot_hp300.0 ${DESTDIR}${MANDIR}8/hp300/boot.0
#	${MINSTALL} boot_i386.0 ${DESTDIR}${MANDIR}8/i386/boot.0
#	${MINSTALL} boot_sparc.0 ${DESTDIR}${MANDIR}8/sparc/boot.0
#	${MINSTALL} boot_tahoe.0 ${DESTDIR}${MANDIR}8/tahoe/boot.0
#	${MINSTALL} boot_vax.0 ${DESTDIR}${MANDIR}8/vax/boot.0

.include <bsd.man.mk>
.include <bsd.prog.mk>

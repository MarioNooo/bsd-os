#	BSDI Makefile,v 2.11 2002/10/18 21:23:43 polk Exp

SUBDIR=	compact termcap

.if ${MACHINE} == "i386" || ${MACHINE} == "sparc" || ${MACHINE} == "sparc_v9"
SUBDIR+=gas
.endif

beforeinstall:
	-@[ -d ${DESTDIR}/usr/old ] || mkdir ${DESTDIR}/usr/old

.include <bsd.subdir.mk>

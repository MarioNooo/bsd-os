#	BSDI Makefile,v 1.1 1996/09/24 22:02:21 donn Exp

INFOFILES=	ld.so.info
CLEANFILES=	${INFOFILES}
INFODIR=	/usr/contrib/info
MAKEINFO=	makeinfo
MAKEINFOFLAGS=	--no-split -I ${.CURDIR} -I .
DIRFILE=	${DESTDIR}/usr/contrib/info/dir
INSTINFO=	install-info --dir-file=${DIRFILE} --quiet \
		--section="System tools" \
		--item="* ld.so: (ld.so).               The dynamic linker."

all: ${INFOFILES}

ld.so.info: ${.CURDIR}/../D/man/ld.so.texi
	${MAKEINFO} ${MAKEINFOFLAGS} -o ${.TARGET} ${.ALLSRC}

depend:

afterinstall:
	-[ -f ${DIRFILE} ] || \
		${MINSTALL} ${.CURDIR}/../../dir ${DIRFILE}
	${INSTINFO} ${INFOFILES}
	${MINSTALL} ld.so.info ${DESTDIR}/usr/contrib/info

.include <bsd.prog.mk>

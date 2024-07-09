#	BSDI Makefile,v 2.21 2001/02/05 16:19:44 donn Exp

MAN1=	shlicc.0
MAN8=	mkshlib.0 shlib.0
LINKS=	/usr/bin/shlicc /usr/bin/shlicc++
MLINKS+=shlicc.1 shlicc++.1 shlicc.1

CFLAGS+=-DRELEASE='"'"$$(uname -r)"'"'

LIBFILES= \
	loader.c

# XXX, but don't want to maintain separate sparc_v9 versions (yet?)
.if ${MACHINE} == "sparc_v9"
MCH=	sparc
.else
MCH=	${MACHINE}
.endif

all mkshlib shlib shlicc: shlicrt0.o ${MAN1} ${MAN8}

depend lint tags:

clean:
	rm -f shlicrt0.o

cleandir: clean
	rm -f ${MAN1} ${MAN8}

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/shlib ${DESTDIR}${BINDIR}/shlib
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/mkshlib ${DESTDIR}${BINDIR}/mkshlib
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/shlicc ${DESTDIR}/usr/bin/shlicc
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444 \
	    shlicrt0.o ${DESTDIR}/usr/lib/shlicrt0.o
	-[ -f ${.CURDIR}/ldscript.${MCH}.shlib ] && \
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444 \
	    ${.CURDIR}/ldscript.${MCH}.shlib \
	    ${DESTDIR}/usr/lib/ldscripts/shlib
	-for i in ${LIBFILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 0444 \
		    ${.CURDIR}/$$i ${DESTDIR}/usr/lib/$$i; \
	done

.include <bsd.prog.mk>

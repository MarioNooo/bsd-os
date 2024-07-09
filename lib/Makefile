#	BSDI Makefile,v 2.23 2003/08/21 00:08:28 polk Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.if ${MACHINE} == "sparc_v9"
MCH=	sparc
.else
MCH=	${MACHINE}
.endif

# missing: libmp libplot
SUBDIR=	libc libcdrom libcompat libdialer libdl libedit \
	libkvm libm librpcsvc libutil liby csu/${MCH}

.if exists(${.CURDIR}/../domestic/Makefile)
# The following require Kerberos to compile as configured
SUBDIR+=libtelnet
.endif

.if	${MCH} == "i386"
SUBDIR+=libcs
.elif	${MCH} == "powerpc"
# nothing yet
.elif	${MCH} == "sparc"
SUBDIR+=libkvmv8 libkvmv9
.endif

# Put in empty libipc.a and librpc.a, so Makefiles loading them don't break.
beforeinstall:
	@(rm -rf /tmp/__placeholder && \
	mkdir /tmp/__placeholder && \
	cd /tmp/__placeholder && \
	as -o __placeholder.o /dev/null && \
	ar cq x.a __placeholder.o && \
	install -c -o ${BINOWN} \
	    -g ${BINGRP} -m 444 x.a ${DESTDIR}/usr/lib/librpc.a && \
	install -c -o ${BINOWN} \
	    -g ${BINGRP} -m 444 x.a ${DESTDIR}/usr/lib/libipc.a && \
	rm -rf /tmp/__placeholder)

.include <bsd.subdir.mk>

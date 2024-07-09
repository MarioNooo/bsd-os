#   BSDI Makefile,v 1.2 1996/12/31 02:31:42 polk Exp

PROG=    stlload
MAN8=    stlload.0
CFLAGS+= -I/sys/i386/isa

stlload.0: stlload.8
	nroff -mdoc $? > $@

beforeinstall:
	-mkdir ${DESTDIR}/usr/libdata/stallion
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${.CURDIR}/cdk.sys ${DESTDIR}/usr/libdata/stallion
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${.CURDIR}/2681.sys ${DESTDIR}/usr/libdata/stallion
	install -c -o root -g wheel -m 444 \
		${.CURDIR}/README.stli ${DESTDIR}/etc/rc.hardware
	install -c -o root -g wheel -m 444 \
		${.CURDIR}/proto.stli ${DESTDIR}/etc/rc.hardware

.include <bsd.prog.mk>


PROG=	v6test
SRCS=	v6test.c cksum.c testcap.c getconfig.c
MAN1=	v6test.0
OWN=-o ${BINOWN} -g ${BINGRP}

#CFLAGS+=-I${SRCDIR}
#CFLAGS+=-DIPSEC
CFLAGS+=-Duint8_t=u_int8_t -Duint16_t=u_int16_t -Duint32_t=u_int32_t
CFLAGS+=-I/usr/contrib/include -DHAVE_LIBPCAP
LDADD+=	-lpcap
DPADD+= /usr/contrib/lib/libpcap.a

#CFLAGS += -DDEFAUT_IF="ep0"
CONFS = ext.conf ndp.conf synloop.conf
PREFIX?= /usr
CONFDIR= ${PREFIX}/share/v6test

.include <bsd.prog.mk>

afterinstall:
	[ -d ${CONFDIR} ] || mkdir -p ${CONFDIR}; \
	for i in ${CONFS}; do \
	    install -c ${OWN} -m 444 ${.CURDIR}/conf/$$i ${CONFDIR}/$$i; \
	done

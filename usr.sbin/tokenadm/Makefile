#	BSDI Makefile,v 1.2 2003/08/20 23:59:28 polk Exp

PROG=	tokenadm
SRCS=	tokenadm.c init.c tokendb.c
CFLAGS+=-I${.CURDIR}/../../libexec/login_token
.PATH:	${.CURDIR}/../../libexec/login_token
TOKENS=	activ crypto snk
LDADD+=	-lcrypto

MAN8=	tokenadm.0
MLINKS=	tokenadm.8 activadm.8
MLINKS+=tokenadm.8 cryptoadm.8
MLINKS+=tokenadm.8 snkadm.8

BINDIR?= /usr/sbin
BINOWN= root
BINMODE=500

afterinstall:
	for i in ${TOKENS} ; do \
	    rm -f ${DESTDIR}/${BINDIR}/$${i}ctl ${DESTDIR}/${BINDIR}/$${i}rm; \
	    install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/tokenctl ${DESTDIR}/${BINDIR}/$${i}ctl ; \
	    install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/tokenrm ${DESTDIR}/${BINDIR}/$${i}rm ; \
	    install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/tokenls ${DESTDIR}/${BINDIR}/$${i}ls ; \
	    cd ${DESTDIR}/${BINDIR} && \
	    rm -f $${i}adm && \
	    ln ${PROG} $${i}adm ; \
	done

.include <bsd.prog.mk>

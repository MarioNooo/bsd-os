#	BSDI Makefile,v 1.8 1997/08/08 19:03:50 prb Exp
PROG=	gettyd
SRCS=	GenSym.c gettyd.c fb.c phone.c util.c uucplock.c
MAN8=	gettyd.0 hardwire.0 hayes.0
MAN5=	dialer.rules.0 ttys.conf.0

.PATH:	${.CURDIR}/../../usr.bin/tip

MLINKS=	hardwire.8 hardwire-call.8
MLINKS+=hardwire.8 hardwire-condition.8
MLINKS+=hardwire.8 hardwire-watcher.8
MLINKS+=hayes.8 hayes-call.8
MLINKS+=hayes.8 hayes-condition.8
MLINKS+=hayes.8 hayes-hangup.8
MLINKS+=hayes.8 hayes-watcher.8

LDADD+=	-ldialer -lutil

SCRIPTS= hayes-condition hardwire-condition
LSCRIPTS=hayes-watcher hayes-call hayes-hangup hardwire-watcher hardwire-call
ETCFILES=ttys.conf dialer.rules
PROTOFILES=ttys.conf.local dialer.rules.local

ETCDIR=	${DESTDIR}/etc
SCRDIR=	${ETCDIR}/getty
SCRMODE=555
DIRMODE=755
BINOWN=root
BINGRP=netdial
OWNERSHIP=-o ${BINOWN} -g ${BINGRP}
CFLAGS	+= -Wall

afterinstall:
	if [ ! -d ${SCRDIR} ] ; then \
	    mkdir -p -m ${DIRMODE} ${SCRDIR} ; \
	    chown ${BINOWN}.${BINGRP} ${SCRDIR} ; \
	fi
	cd ${.CURDIR} && \
	install ${COPY} ${OWNERSHIP} -m ${SCRMODE} ${SCRIPTS} ${SCRDIR}
	cd ${.CURDIR} && \
	install -c ${OWNERSHIP} -m 444 ${ETCFILES} ${ETCDIR} ;\
	cd ${.CURDIR} && \
	for i in ${PROTOFILES} ; do \
		if [ ! -f ${ETCDIR}/$$i ] ; then \
			install -c ${OWNERSHIP} -m 644 $$i ${ETCDIR} ; \
		fi \
	done
	cd ${SCRDIR} && \
	rm -f ${LSCRIPTS} && \
	for i in ${LSCRIPTS} ; do \
		ln $$(echo $$i | sed s/-.*//)-condition $$i ; \
	done

.include <bsd.prog.mk>

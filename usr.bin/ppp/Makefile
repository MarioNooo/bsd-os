#	BSDI Makefile,v 2.15 2001/04/13 15:29:43 dab Exp

PROG=   ppp
CFLAGS+=-Wall
CFLAGS+=-I${.CURDIR} -I/sys -fwritable-strings
DPADD=	${LIBUTIL}
LDADD=	-lutil -ldialer
UTILS=	ppputil
CLEANFILES+= *.0 ${UTILS} ppputil.o
BINOWN=	root
BINGRP=	netdial
BINMODE=4550
SCRMODE=550
FILEMODE=440
ETCFILEMODE=400
DIRMODE=0750
MAN8=	ppp.0
MAN5=	ppp.sys.0
SRCS=	pbuf.c ppp_fsm.c ppp_lcp.c ppp_ipcp.c ppp_pap.c ppp_util.c util.c
SRCS+=	authenticate.c ppp_chap.c ppp_chap_subr.c
SRCS+=	dial.c ppp.c readsys.c setlinktype.c
SRCS+=	ppp_ipv6cp.c

SCRIPTS=Dialout.login Dialout.up Dialout.down \
	Demand.login Demand.up Demand.down \
	Dialin.login Dialin.up Dialin.down \
	addr-map.sample
OPROGS=	config_dialin config_ppp
ETCFILES=ppp.sys.sample

ETCDIR=	${DESTDIR}/etc
UTILDIR=${DESTDIR}/usr/libexec
SBINDIR=${DESTDIR}/usr/sbin
SCRDIR=	${ETCDIR}/netscripts
SKELDIR=${SCRDIR}/SKEL
HOMEDIR=${SCRDIR}/HOME

# -- acutab is configuration dependent, and so depends on the Makefile

all:	${PROG} ${UTILS} ${MAN5} ${MAN8}

acutab.o: Makefile

ppputil: ppputil.o
	${LDCC} ${LDFLAGS} -o ${.TARGET} ppputil.o ${LDADD}

OWNERSHIP=-o ${BINOWN} -g ${BINGRP}

afterinstall:
	install ${COPY} ${STRIP} ${OWNERSHIP} -m ${SCRMODE} ${INSTALLFLAGS} \
	    ${UTILS} ${UTILDIR}
	if [ ! -d ${SCRDIR} ] ; then \
	    mkdir -p -m ${DIRMODE} ${SCRDIR} ; \
	    chown ${BINOWN}.${BINGRP} ${SCRDIR} ; \
	fi
	if [ ! -d ${SKELDIR} ] ; then \
	    mkdir -p -m ${DIRMODE} ${SKELDIR} ; \
	    chown ${BINOWN}.${BINGRP} ${SKELDIR} ; \
	fi
	if [ ! -d ${HOMEDIR} ] ; then \
	    mkdir -p -m ${DIRMODE} ${HOMEDIR} ; \
	    chown ${BINOWN}.${BINGRP} ${HOMEDIR} ; \
	fi
	cd ${.CURDIR} && \
	install ${COPY} ${OWNERSHIP} -m ${SCRMODE} ${OPROGS} ${SBINDIR}
	cd ${.CURDIR} && \
	install ${COPY} ${OWNERSHIP} -m ${SCRMODE} ${SCRIPTS} ${SCRDIR}
	cd ${.CURDIR} && \
	install ${COPY} ${OWNERSHIP} -m ${ETCFILEMODE} ${ETCFILES} ${ETCDIR}
	cd ${.CURDIR} && \
	if [ ! -f ${SKELDIR}/dot.forward ] ; then \
	    install ${COPY} ${OWNERSHIP} -m ${FILEMODE} \
		dot.forward ${SKELDIR} ; \
	fi
	cd ${.CURDIR} && \
	if [ ! -f ${HOMEDIR}/.forward ] ; then \
	    install ${COPY} ${OWNERSHIP} -m ${FILEMODE} \
		dot.forward ${HOMEDIR}/.forward ; \
	fi
	[ -f ${ETCDIR}/ppp.sys ] || \
	    cp -p ${ETCDIR}/ppp.sys.sample ${ETCDIR}/ppp.sys
	[ -f ${SCRDIR}/addr-map ] || \
	    cp -p ${SCRDIR}/addr-map.sample ${SCRDIR}/addr-map

.include <bsd.prog.mk>

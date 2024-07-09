#	BSDI Makefile.inc,v 2.3 1996/09/04 21:56:24 polk Exp

.ifndef (Mk.Inc)
Mk.Inc?=defined

DATE!=		date +%y/%m/%d
OS!=		uname -s
REL!=		uname -r
VERSION=	'-DVERSION="\"${OS} ${REL} ${DATE}\""'

CONFIG?=	-DFCNTL -DUSE_TERMIOS -DBSDTCP -DHAYES -DHAYESV -DPROG \
		-DNFS -DSAFE_LIBC_INET_NTOA
INCLUDE?=	-I. -I${.CURDIR}/../i -I${.CURDIR}/../args
CFLAGS=		-O -DANSI_C ${INCLUDE} ${CONFIG} ${VERSION}

BINGRP=		uucp
BINOWN=		uucp

CLEANFILES+=	*.0

# Make sure that these match their definition in i/site.h
LOGDIR?=	/var/log/uucp
PARAMSFILE?=	/etc/uucp/CONFIG
PARAMSDIR?=	/etc/uucp
PUBDIR?=	/var/spool/uucppublic

.if defined(PROG)
LDADD+=         ../args/libargs.a ../libacu/libacu.a ../lib/lib.a \
	../libc/libc.a
.endif
                
.endif

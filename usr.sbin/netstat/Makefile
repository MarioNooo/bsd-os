#	@(#)Makefile	8.1 (Berkeley) 6/12/93

PROG=	netstat
SRCS=	if.c if_util.c inet.c inet6.c ipsec.c main.c mbuf.c \
	mroute.c route.c unix.c mroute6.c
CFLAGS+=-I${.CURDIR}

SYSDIR=${.CURDIR}/../../sys

.if exists(${SYSDIR}/netiso/tp_astring.c)
.PATH:	${SYSDIR}/netiso
SRCS+= iso.c tp_astring.c
CFLAGS+= -DISO
.endif

.if exists(${SYSDIR}/netns)
SRCS+= ns.c
CFLAGS+= -DXNS
.endif

.if exists(${SYSDIR}/netccitt)
CFLAGS+= -DX25
.endif

CFLAGS+= -I${SYSDIR}

BINGRP=	kmem
BINMODE=2555
LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>

#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ifconfig
MAN8=	ifconfig.0

SYSDIR=${.CURDIR}/../../sys.dev
CFLAGS+=-I${SYSDIR} -I${SYSDIR}/i386

.if exists(${SYSDIR}/netccitt/x25.h)
SRCS+= ccitt_addr.c
CFLAGS+= -DX25
.endif

.if exists(${SYSDIR}/netns/ns.h)
CFLAGS+= -DXNS
.endif

.if exists(${SYSDIR}/netiso/iso.h)
CFLAGS+= -DISO
.endif

# KAME scopeid hack
CFLAGS+=-DKAME_SCOPEID

.include <bsd.prog.mk>

#	BSDI Makefile,v 2.4 1996/09/19 15:42:28 polk Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	kdump
CFLAGS+=-I${.CURDIR}/../ktrace -I/sys
SRCS=	kdump.c ioctl.c subr.c
.PATH:	${.CURDIR}/../ktrace
CLEANFILES+=ioctl.c
IOCTLS=	/usr/include/sys/filio.h /usr/include/sys/ioccom.h \
	/usr/include/sys/ioctl.h /usr/include/sys/ioctl_compat.h \
	/usr/include/sys/sockio.h /usr/include/sys/ttycom.h

ioctl.c: mkioctls
	/bin/sh ${.CURDIR}/mkioctls ${IOCTLS} > ioctl.c

.include <bsd.prog.mk>

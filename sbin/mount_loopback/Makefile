#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_loopback
SRCS=	mount_loopback.c getmntopts.c
MAN8=	mount_loopback.0

MLINKS=	mount_loopback.8 mount_null.8
MOUNT=	${.CURDIR}/../mount
CFLAGS+=-I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>

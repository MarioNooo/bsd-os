#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_romfs
SRCS=	mount_romfs.c getmntopts.c
MAN8=	mount_romfs.0

MOUNT=	${.CURDIR}/../mount
.if !exists(${MOUNT}/mntopts.h)
MOUNT=	${.CURDIR}/../../../sbin/mount
.if !exists(${MOUNT}/mntopts.h)
MOUNT=	${.CURDIR}/../mount
.endif
.endif

CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>

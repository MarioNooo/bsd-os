#	$NetBSD: Makefile,v 1.9 1995/03/18 14:57:35 cgd Exp $

PROG=	mount_msdos
SRCS=	mount_msdos.c getmntopts.c
MAN8=	mount_msdos.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>

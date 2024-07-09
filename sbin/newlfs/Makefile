#	BSDI	Makefile,v 2.2 1996/06/12 15:20:07 bostic Exp
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

PROG=	newlfs
SRCS=	lfs.c lfs_cksum.c misc.c newfs.c
MAN8=	newlfs.0

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.PATH:	/sys/ufs/lfs

.include <bsd.prog.mk>

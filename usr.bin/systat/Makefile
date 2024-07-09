#	BSDI Makefile,v 2.4 1997/10/31 04:16:32 jch Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	systat
SRCS=	cmds.c cmdtab.c disks.c fetch.c if_util.c iostat.c keyboard.c vmstat.c \
	main.c mbufs.c netstat.c pigs.c swap.c
LDADD=	-lcurses -ltermcap -lm -lkvm
DPADD=	${LIBCURSES} ${LIBTERMCAP} ${LIBM}
BINGRP=	kmem
BINMODE=2555
.PATH:	${.CURDIR}/../../usr.sbin/netstat
CFLAGS+=	-I${.CURDIR}/../../usr.sbin/netstat

.include <bsd.prog.mk>

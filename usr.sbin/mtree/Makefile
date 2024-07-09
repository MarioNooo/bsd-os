#	BSDI Makefile,v 2.2 1996/06/04 15:53:59 bostic Exp
#
#	@(#)Makefile	8.2 (Berkeley) 4/27/95

PROG=	mtree
MAN8=	mtree.0
#CFLAGS+=-DDEBUG
SRCS=	compare.c crc.c create.c misc.c mtree.c spec.c verify.c
.PATH:	${.CURDIR}/../../usr.bin/cksum

.include <bsd.prog.mk>

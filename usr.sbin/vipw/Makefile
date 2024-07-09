#	BSDI Makefile,v 2.2 1998/04/25 02:30:02 prb Exp
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	vipw
SRCS=	vipw.c crc.c
.PATH:	${.CURDIR}/../../usr.bin/cksum
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
MAN8=	vipw.0

.include <bsd.prog.mk>

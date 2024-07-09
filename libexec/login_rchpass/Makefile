#	BSDI Makefile,v 1.2 1996/08/26 15:19:51 prb Exp

PROG=	login_rchpass
SRCS=	login_rchpass.c rpasswd.c lib.c
MAN8=	login_rchpass.0

CFLAGS+=-I${.CURDIR}/../../usr.bin/rpasswd
.PATH:	${.CURDIR}/../../usr.bin/rpasswd

.include <bsd.prog.mk>

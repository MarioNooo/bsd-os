#	BSDI Makefile,v 1.2 1996/08/26 15:20:30 prb Exp

PROG=	rpasswdadm
SRCS=	rpasswdadm.c lib.c
MAN8=	rpasswdadm.0

CFLAGS+=-I${.CURDIR}/../../usr.bin/rpasswd
.PATH:	${.CURDIR}/../../usr.bin/rpasswd

.include <bsd.prog.mk>

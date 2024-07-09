#	BSDI Makefile,v 1.2 1996/08/26 15:19:20 prb Exp

PROG=	login_rpasswd
SRCS=	login_rpasswd.c lib.c
BINOWN= root
BINMODE=4555
MAN8=	login_rpasswd.0

CFLAGS+=-I${.CURDIR}/../../usr.bin/rpasswd
.PATH:  ${.CURDIR}/../../usr.bin/rpasswd

.include <bsd.prog.mk>

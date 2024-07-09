#	BSDI	Makefile,v 1.2 1997/08/08 18:58:22 prb Exp

PROG=	login_lchpass
SRCS=	local_passwd.c login_lchpass.c
MAN8=	login_lchpass.0
# .PATH:	${.CURDIR}/../../usr.bin/passwd

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>

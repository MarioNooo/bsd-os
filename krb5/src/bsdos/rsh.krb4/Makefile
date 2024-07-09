#	BSDI	Makefile,v 2.6 1999/11/04 05:59:37 dab Exp

BINDIR=	/usr/bin
PROG=	rsh
SRCS=	rsh.c krcmd.c kcmd.c krb_configured.c
CFLAGS+=-DKERBEROS -I${.CURDIR}/../rlogin.krb4
CFLAGS+=-I/usr/include/krb5 -I/usr/include/krb5/kerberosIV
DPADD=	${LIBKRB} ${LIBDES}
LDADD=	-lkrb4 -ldes425 -lkrb5 -lk5crypto -lprofile -lcom_err

.if !defined(NOCRYPT)
SRCS+=	des_rw.c
CFLAGS+=-DCRYPT
.endif

BINOWN=	root
BINMODE=4555
.PATH:	${.CURDIR}/../rlogin.krb4

.include <bsd.prog.mk>

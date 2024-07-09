#	BSDI	Makefile,v 2.5 1999/11/04 05:59:35 dab Exp
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

BINDIR=	/bin
PROG=	rcp
SRCS=	rcp.c util.c krcmd.c kcmd.c krb_configured.c
CFLAGS+=-DKERBEROS -I${.CURDIR}/../rlogin.krb4
CFLAGS+=-I/usr/include/krb5 -I/usr/include/krb5/kerberosIV
DPADD=	${LIBKRB} ${LIBDES}
LDADD=	-lkrb4 -ldes425 -lkrb5 -lk5crypto -lprofile -lcom_err

.if !defined(NOENCRYPT)
SRCS+=	des_rw.c
CFLAGS+=-DCRYPT
.endif

BINOWN=	root
BINMODE=4555
.PATH:	${.CURDIR}/../rlogin.krb4

.include <bsd.prog.mk>

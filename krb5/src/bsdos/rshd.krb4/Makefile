#	BSDI Makefile,v 2.7 1999/11/16 20:58:09 dab Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

BINDIR=	/usr/libexec
PROG=	rshd
SRCS=	rshd.c krb_configured.c k_getsockinst.c
MAN8=	rshd.0

CFLAGS+=-DKERBEROS -I${.CURDIR}/../rlogin.krb4
CFLAGS+=-I/usr/include/krb5 -I/usr/include/krb5/kerberosIV
LDADD=	-lkrb4 -ldes425 -lkrb5 -lk5crypto -lprofile -lcom_err -lutil

.if !defined(NOCRYPT)
CFLAGS+=-DCRYPT
SRCS+= des_rw.c
.endif

.PATH: ${.CURDIR}/../rlogin.krb4

.include <bsd.prog.mk>

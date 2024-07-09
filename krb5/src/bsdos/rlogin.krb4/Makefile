#	BSDI	Makefile,v 2.6 1999/11/04 05:59:37 dab Exp

BINDIR=	/usr/bin
PROG=	rlogin
SRCS=	rlogin.c krcmd.c kcmd.c krb_configured.c
CFLAGS+=-DKERBEROS
CFLAGS+=-I/usr/include/krb5 -I/usr/include/krb5/kerberosIV
LDADD=  -lkrb4 -ldes425 -lkrb5 -lk5crypto -lprofile -lcom_err

.if !defined(NOCRYPT)
SRCS+=	des_rw.c
CFLAGS+=-DCRYPT
.endif

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>

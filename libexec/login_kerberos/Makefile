#	BSDI	Makefile,v 1.6 2003/08/21 22:29:27 polk Exp

BINDIR=	/usr/libexec

PROG=	login_kerberos
SRCS=	klogin.c login_kerberos.c krb_configured.c
NOMAN=

CFLAGS+=-DKERBEROS -DKRB5_KRB4_COMPAT -I/usr/include/krb5
DPADD+= ${LIBKRB} ${LIBDES} ${LIBUTIL}
LDADD+= -lkrb4 -lkrb524 -ldes425 -lkrb5 -lk5crypto 
LDADD+=	-lprofile -lcom_err -lutil
BINOWN=	root
BINMODE=4555

.PATH:	${.CURDIR}/../../krb5/src/bsdos/rlogin.krb4

.include <bsd.prog.mk>

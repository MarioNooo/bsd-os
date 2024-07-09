#	BSDI	Makefile,v 1.8 2003/08/21 22:29:27 polk Exp

PROG=	login_krb-or-pwd
SRCS=	klogin.c login_kerberos.c krb_configured.c
.PATH:	${.CURDIR}/../login_kerberos
CFLAGS+=-DPASSWD -DKERBEROS
CFLAGS+=-DKRB5_KRB4_COMPAT -I/usr/include/krb5
DPADD+= ${LIBKRB} ${LIBDES} ${LIBUTIL}
LDADD+= -lkrb4 -lkrb524 -ldes425 -lkrb5 -lk5crypto 
LDADD+=	-lprofile -lcom_err -lutil
NOMAN=

BINOWN=	root
BINMODE=4555

.PATH:	${.CURDIR}/../../krb5/src/bsdos/rlogin.krb4

.include <bsd.prog.mk>

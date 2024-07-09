#	BSDI	Makefile,v 2.5 2003/08/21 22:29:28 polk Exp
#	@(#)Makefile	8.3 (Berkeley) 4/2/94

PROG=	passwd
SRCS=	des_rw.c krb_passwd.c local_passwd.c passwd.c krb_configured.c
CFLAGS+=-I${.CURDIR}/../rlogin -I/usr/include/krb5
CFLAGS+=-DKERBEROS -DKRB5_KRB4_COMPAT -DCRYPT
DPADD=	${LIBKRB} ${LIBDES} ${LIBUTIL}
LDADD=	-lkrb4 -lkrb524 -ldes425 -lkrb5 -lk5crypto -lprofile -lcom_err -lutil

.PATH:	${.CURDIR}/../../domestic/usr.bin/rlogin
.PATH:	${.CURDIR}/../../krb5/src/bsdos/rlogin.krb4

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>

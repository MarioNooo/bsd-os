#	BSDI	Makefile,v 1.3 2003/08/21 22:29:27 polk Exp

BINDIR=	/usr/libexec

PROG=	login_chpass
SRCS=	klogin.c login_chpass.c
SRCS+=   des_rw.c krb_passwd.c krb_configured.c
MAN8=	login_chpass.0
.PATH:	${.CURDIR}/../login_kerberos ${.CURDIR}/../../krb5/src/bsdos/rlogin.krb4
.PATH:  ${.CURDIR}/../../usr.bin/passwd ${.CURDIR}/../../domestic/usr.bin/rlogin

CFLAGS+=-DKERBEROS -DKRB5_KRB4_COMPAT -DCRYPT -I/usr/include/krb5
CFLAGS+=-I${.CURDIR}/../../usr.bin/rlogin
DPADD+= ${LIBKRB} ${LIBDES}
LDADD+= -lkrb4 -lkrb524 -ldes425 -lkrb5 -lk5crypto -lprofile -lcom_err

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>

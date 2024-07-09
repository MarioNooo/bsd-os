#	BSDI	Makefile,v 1.4 1996/03/01 19:19:17 prb Exp

BINDIR=	/usr/libexec
PROG=	login_krb5-or-pwd
SRCS=	k5login.c login_krb5.c
NOMAN=

.PATH: ${.CURDIR}/../login_krb5

KRB5INCS=/usr/local/krb5/include
KRB5INCS=/usr/include/krb5

KRB5LIBS=/usr/local/krb5/lib
KRB5LIBS=/usr/lib

CFLAGS+=-DKRB5 -DKRB5_VERIFY -I${KRB5INCS}
CFLAGS+=-DPASSWD
CFLAGS+=-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/../../include/krb5 -I${.CURDIR}/../../util/et \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj
#DPADD+= ${LIBKRB} ${LIBDES} ${LIBUTIL}

.if exists(${KRB5LIBS}/libkrb5.so)
# Use dynamic shared krb5 libraries...
LDADD+=	-L${KRB5LIBS} -Wl,-rpath,${KRB5LIB} \
	-lkrb5 -lk5crypto -lprofile -lcom_err -lutil
LDCC=	${CC}
.else
# Use static krb5 libraries
LDADD+= -L${KRB5LIBS} -lkrb5 -lk5crypto -lprofile ${KRB5LIBS}/libcom_err.a \
	-lutil
.endif

BINOWN=	root
BINMODE=4555
# INSTALLFLAGS=-fschg

.include <bsd.prog.mk>

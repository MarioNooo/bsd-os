#	BSDI makefile,v 1.5 2003/08/20 23:26:15 polk Exp

SUBDIR=	../../domestic/libexec/identd/idecrypt

.PATH:	${.CURDIR}/src ${.CURDIR}/../../domestic/libexec/identd/src
CFLAGS+=-I${.CURDIR}/../../domestic/libexec/identd/src

PROG=	identd
SRCS=	\
	identd.c	\
	parse.c		\
	kvm.c		\
	version.c	\
	proxy.c		\
	config.c	\
	crypto.c	\
	bsdi.c
 
MAN8=	identd.0
CFLAGS+=-DBSDI -DHAVE_KVM -DINCLUDE_EXTENSIONS -DSTRONG_LOG -DALLOW_FORMAT \
	-DDPATH_CONFIG='"/etc/identd.conf"' -DPATH_DESKEY='"/etc/identd.key"'
LDADD=	-lutil -lkvm
DPADD=	${LIBUTIL} ${LIBKVM}

.if !defined(NOCRYPT)
CFLAGS+= -DINCLUDE_CRYPT -Ddes_set_key=des_setkey -I/usr/include/kerberosIV
LDADD+=	-lcrypto
DPADD+= ${LIBDES}
.endif

.include <bsd.prog.mk>

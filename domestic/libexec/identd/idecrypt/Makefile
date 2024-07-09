#	BSDI Makefile,v 1.3 2003/08/20 23:18:14 polk Exp

BINDIR?=	/usr/sbin

.PATH:	${.CURDIR}/../../../../libexec/identd/src ${.CURDIR}/../src
CFLAGS+=-I${.CURDIR}/../src

PROG=	idecrypt
SRCS=	\
	idecrypt.c	\
	crypto.c

MAN8=	idecrypt.0
 
CFLAGS+=-DBSDI -DHAVE_KVM -DINCLUDE_EXTENSIONS -DSTRONG_LOG -DALLOW_FORMAT \
	-DINCLUDE_CRYPT -Ddes_set_key=des_setkey -I/usr/include/kerberosIV \
	-DDPATH_CONFIG='"/etc/identd.conf"' -DPATH_DESKEY='"/etc/identd.key"'

LDADD=	-lcrypto
DPADD=	${LIBDES}

.include <bsd.prog.mk>

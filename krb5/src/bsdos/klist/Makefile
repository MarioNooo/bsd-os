BINDIR=	/usr/bin
PROG=	klist

SRCS=	klist.c

.PATH:	${.CURDIR}/../../clients/klist

CFLAGS+=-I. -I${DESTDIR}/usr/include/krb5
CFLAGS+=-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/../../include/krb5 \
	-I${.CURDIR}/../../util/et \
	-I${.CURDIR}/../../lib \
	-I${.CURDIR}/../../util -I${.CURDIR}/../../util/ss/obj \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj

CFLAGS+=-DHAVE_STDARG_H=1 -DHAVE_GETUSERSHELL=1 -DHAVE_LSTAT=1 \
	-DHAVE_SETENV=1 -DHAVE_UNSETENV=1 -DHAVE_GETENV=1 \
	-DHAVE_INET_NTOP=1 -DHAVE_GETIPNODEBYADDR=1 -DHAVE_UNISTD_H=1 \
	-DHAVE_PWD_H=1 -DHAVE_ARPA_INET_H=1 

LDADD+=	-lkrb4 -ldes425 -lkrb5 -lk5crypto -lss -lprofile -lcom_err

.depend: klist.1
CLEANFILES+= klist.1

klist.1: ${.CURDIR}/../../clients/klist/klist.M
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.prog.mk>

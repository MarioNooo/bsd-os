BINDIR=	/usr/bin
PROG=	kdestroy

SRCS=	kdestroy.c

.PATH:	${.CURDIR}/../../clients/kdestroy

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

.depend: kdestroy.1
CLEANFILES+= kdestroy.1

kdestroy.1: ${.CURDIR}/../../clients/kdestroy/kdestroy.M
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.prog.mk>

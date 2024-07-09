BINDIR=	/usr/sbin
PROG=	v5passwd

SRCS=	adm_rw.c kpasswd.c adm_conn.c

.PATH:	${.CURDIR}/../../kadmin/v5passwdd

CFLAGS+=-I. -I${DESTDIR}/usr/include/krb5
CFLAGS+=-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/../../include/krb5 \
	-I${.CURDIR}/../../util/et \
	-I${.CURDIR}/../../lib \
	-I${.CURDIR}/../../util -I${.CURDIR}/../../util/ss/obj \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj

CFLAGS+=-DHAVE_UNISTD_H=1 -DHAVE_STDLIB_H=1 -DHAVE_SYS_TIME_H=1 \
	-DHAVE_SYS_SELECT_H=1 -DHAVE_TIMEZONE=1 -DHAVE_GETCWD=1 \
	-DHAVE_STRSTR=1 -DHAVE_WAITPID=1 -DHAVE_VSPRINTF=1 \
	-DHAVE_MEMMOVE=1 -DHAVE_STRFTIME=1 -DTIME_WITH_SYS_TIME=1 \
	-DPOSIX_SIGNALS=1 -DPOSIX_SETJMP=1 -DHAVE_TCL_H=1 

LDADD+=	-lkadm5clnt \
	-lgssrpc -ldyn -lgssapi_krb5 -lkrb5 -lk5crypto \
	-lss -lprofile -lcom_err

.depend: v5passwd.1
CLEANFILES+= v5passwd.1

v5passwd.1: ${.CURDIR}/../../kadmin/v5passwdd/v5passwd.M
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.prog.mk>

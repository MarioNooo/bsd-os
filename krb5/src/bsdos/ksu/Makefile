BINDIR=	/usr/bin
PROG=	ksu
BINOWN=	root
BINMODE=4555

SRCS=	krb_auth_su.c ccache.c authorization.c main.c heuristic.c xmalloc.c

.PATH:	${.CURDIR}/../../clients/ksu

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
CFLAGS+=-DGET_TGT_VIA_PASSWD -DPRINC_LOOK_AHEAD -DCMD_PATH='"/bin /usr/bin"'

LDADD+=	-lkrb5 -lkrb5util -lk5crypto -lss -lprofile -lcom_err

.depend: ksu.1
CLEANFILES+= ksu.1

ksu.1: ${.CURDIR}/../../clients/ksu/ksu.M
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.prog.mk>

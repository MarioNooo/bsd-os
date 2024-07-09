BINDIR=	/usr/libexec
PROG=	kadmind
MAN8=	kadmind.0

SRCS=	kadm_rpc_svc.c server_stubs.c ovsec_kadmd.c schpw.c misc.c \
	server_glue_v1.c

.PATH:	${.CURDIR}/../../kadmin/server

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

LDADD+= -lkadm5srv -lkdb5 \
	-L${.CURDIR}/../../util/db2 -L${.CURDIR}/../../util/db2/obj -ldb \
	-lgssrpc -ldyn -lgssapi_krb5 \
	-lkrb4 -lkrb5 -ldes425 -lk5crypto -lprofile -lcom_err

.depend: kadm5 gssrpc gssapi gssapi.h gssapi_krb5.h \
	kadm_err.h adb_err.h chpass_util_strings.h server_acl.h
CLEANFILES+= kadm5 gssrpc gssapi gssapi.h gssapi_krb5.h \
	kadm_err.h adb_err.h chpass_util_strings.h server_acl.h

kadm5:
	if [ ! -h kadm5 ]; then \
		ln -s . kadm5 ; \
	fi

kadm_err.h:
	if [ -d ${.CURDIR}/../../lib/kadm5/clnt/obj ]; then \
		ln -s ${.CURDIR}/../../lib/kadm5/clnt/obj/kadm_err.h ; \
	else \
		ln -s ${.CURDIR}/../../lib/kadm5/clnt/kadm_err.h ; \
	fi

adb_err.h:
	if [ -d ${.CURDIR}/../../lib/kadm5/clnt/obj ]; then \
		ln -s ${.CURDIR}/../../lib/kadm5/clnt/obj/adb_err.h ; \
	else \
		ln -s ${.CURDIR}/../../lib/kadm5/clnt/adb_err.h ; \
	fi

chpass_util_strings.h:
	if [ -d ${.CURDIR}/../../lib/kadm5/clnt/obj ]; then \
		ln -s ${.CURDIR}/../../lib/kadm5/clnt/obj/chpass_util_strings.h ; \
	else \
		ln -s ${.CURDIR}/../../lib/kadm5/clnt/chpass_util_strings.h ; \
	fi

gssrpc:
	if [ ! -h gssrpc ]; then \
		ln -s ${.CURDIR}/../../lib/rpc gssrpc ; \
	fi

gssapi:
	if [ ! -h gssapi ]; then \
		ln -s . gssapi ; \
	fi

gssapi.h:
	if [ ! -h gssapi.h ]; then \
		if [ -d ${.CURDIR}/../../lib/gssapi/obj ]; then \
			ln -s ${.CURDIR}/../../lib/gssapi/obj/gssapi.h ; \
		else \
			ln -s ${.CURDIR}/../../lib/gssapi/gssapi.h ; \
		fi \
	fi

gssapi_krb5.h:
	if [ ! -h gssapi_krb5.h ]; then \
		ln -s ${.CURDIR}/../../lib/gssapi/krb5/gssapi_krb5.h ; \
	fi

server_acl.h:
	if [ ! -h server_acl.h ]; then \
		ln -s ${.CURDIR}/../../lib/kadm5/srv/server_acl.h ; \
	fi

kadmind.8: ${.CURDIR}/../../kadmin/server/kadmind.M
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.prog.mk>

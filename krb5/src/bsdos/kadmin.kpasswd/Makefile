BINDIR=	/usr/sbin
PROG=	kpasswd

SRCS=	tty_kpasswd.c kpasswd.c kpasswd_strings.c

.PATH:	${.CURDIR}/../../kadmin/passwd

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
CFLAGS+=-DUSE_KADM5_API_VERSION=1

LDADD+=	-lkadm5clnt \
	-lgssrpc -ldyn -lgssapi_krb5 -lkrb5 -lk5crypto \
	-lss -lprofile -lcom_err

.depend: kpasswd.1 kpasswd_strings.c kpasswd_strings.h \
	kadm5 gssrpc gssapi gssapi.h gssapi_krb5.h \
	kadm_err.h adb_err.h chpass_util_strings.h
CLEANFILES+= kpasswd_strings.c kpasswd_strings.h \
	kadm5 gssrpc gssapi gssapi.h gssapi_krb5.h \
	kadm_err.h adb_err.h chpass_util_strings.h

kpasswd.1: ${.CURDIR}/../../kadmin/passwd/kpasswd.M
	cp ${.ALLSRC} ${.TARGET}
kpasswd_strings.c: ${.CURDIR}/../../kadmin/passwd/kpasswd_strings.et
	awk -f /usr/share/krb5/compile_et/et_c.awk outfile=${.TARGET} ${.ALLSRC}
kpasswd_strings.h: ${.CURDIR}/../../kadmin/passwd/kpasswd_strings.et
	awk -f /usr/share/krb5/compile_et/et_h.awk outfile=${.TARGET} ${.ALLSRC}

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

.include <bsd.prog.mk>

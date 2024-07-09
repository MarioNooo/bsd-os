BINDIR=	/usr/sbin
PROG=	ktutil
MAN8=	ktutil.0

SRCS=	ktutil.c ktutil_ct.c ktutil_funcs.c

.PATH:	${.CURDIR}/../../kadmin/ktutil

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

LDADD+= -lss -lkrb4 -lkrb5 -ldes425 -lk5crypto -lprofile -lcom_err

.depend: ktutil.8 ktutil_ct.c kadm5 gssrpc
CLEANFILES+= ktutil.8 ktutil_ct.c import_err.c kadm5 gssrpc

ktutil.8: ${.CURDIR}/../../kadmin/ktutil/ktutil.M
	cp ${.ALLSRC} ${.TARGET}

ktutil_ct.c: ${.CURDIR}/../../kadmin/ktutil/ktutil_ct.ct
	/usr/share/krb5/mk_cmds ${.ALLSRC}

kadm5:
	if [ ! -h kadm5 ]; then \
		if [ -d ${.CURDIR}/../../lib/kadm5/clnt/obj ]; then \
			ln -s ${.CURDIR}/../../lib/kadm5/clnt/obj kadm5 ; \
		else \
			ln -s ${.CURDIR}/../../lib/kadm5/clnt kadm5 ; \
		fi \
	fi

gssrpc:
	if [ ! -h gssrpc ]; then \
		ln -s ${.CURDIR}/../../lib/rpc gssrpc ; \
	fi

.include <bsd.prog.mk>

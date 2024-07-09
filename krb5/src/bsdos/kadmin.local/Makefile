BINDIR=	/usr/sbin
PROG=	kadmin.local
MAN8=	kadmin.local.0

SRCS=	kadmin.c kadmin_ct.c ss_wrapper.c getdate.c keytab.c

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

# kadmin.local: $(SS_LIB) $(KADMSRV_LIBS) $(KRB5_BASE_LIBS)
# KRB5_BASE_LIBS: -lkrb5 -lk5crypto -lcom_err
# KADMSRV_LIBS: -lkadm5srv -lkdb5 -ldb -lgssrpc -ldyn -lgssapi_krb5

LDADD+=	-lkadm5srv -lkdb5 \
	-L${.CURDIR}/../../util/db2 -L${.CURDIR}/../../util/db2/obj -ldb \
	-lgssrpc -ldyn -lgssapi_krb5 -lkrb5 -lk5crypto \
	-lss -lprofile -lcom_err

.PATH:	${.CURDIR}/../../kadmin/cli

.depend: kadmin.local.8 kadmin_ct.c kadm5 gssrpc

CLEANFILES+= kadmin.local.8 kadmin_ct.c kadm5 gssrpc

# how do I hate sleazy tricks with man pages? let me count the ways...
kadmin.local.8: ${.CURDIR}/../../kadmin/cli/kadmin.M
	cp ${.ALLSRC} ${.TARGET}

kadmin_ct.c: ${.CURDIR}/../../kadmin/cli/kadmin_ct.ct
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

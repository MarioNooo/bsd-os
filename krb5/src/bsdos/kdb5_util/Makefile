BINDIR=	/usr/sbin
PROG=	kdb5_util
MAN8=	kdb5_util.0

SRCS=	kdb5_util.c dump.c dumpv4.c loadv4.c \
	kdb5_create.c kadm5_create.c string_table.c kdb5_stash.c \
	kdb5_destroy.c ovload.c import_err.c strtok.c

.PATH:	${.CURDIR}/../../kadmin/dbutil

CFLAGS+=-I. -I${DESTDIR}/usr/include/krb5
CFLAGS+=-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/../../include/kerberosIV \
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
CFLAGS+=-DKDB4_DISABLE

LDADD+=	-lkadm5srv -lkdb5 \
	-L${.CURDIR}/../../util/db2 -L${.CURDIR}/../../util/db2/obj -ldb \
	-lgssrpc -ldyn -lgssapi_krb5 \
	-lkrb4 -lkrb5 -ldes425 -lk5crypto -lprofile -lcom_err

.depend: kdb5_util.8 import_err.h import_err.c kadm5 gssrpc
CLEANFILES+= kdb5_util.8 import_err.h import_err.c kadm5 gssrpc

kdb5_util.8: ${.CURDIR}/../../kadmin/dbutil/kdb5_util.M
	cp ${.ALLSRC} ${.TARGET}
import_err.h: ${.CURDIR}/../../kadmin/dbutil/import_err.et
	awk -f ${.CURDIR}/../../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}
import_err.c: ${.CURDIR}/../../kadmin/dbutil/import_err.et
	awk -f ${.CURDIR}/../../util/et/et_c.awk outfile=${.TARGET} ${.ALLSRC}

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

BINDIR=	/usr/bin
PROG=	k524init
NOMAN=

CFLAGS+=-I. -I${DESTDIR}/usr/include/krb5
CFLAGS+=-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/../../include/krb5 -I${.CURDIR}/../../include/kerberosIV\
	-I${.CURDIR}/../../util/et \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj
CFLAGS+=-I${.CURDIR}/../../krb524/obj -I${.CURDIR}/../../krb524

CFLAGS+=-DHAVE_SYS_SELECT_H=1 -DRETSIGTYPE=void

# k524init: $(KRB524_LIB) $(KRB4COMPAT_LIBS)

# KRB524_LIB
LDADD+= -lkrb524
# KRB4COMPAT_LIBS
LDADD+=	-lkrb4 -lkrb5 -ldes425 -lk5crypto -lprofile -lcom_err

.PATH:	${.CURDIR}/../../krb524

.depend: kadm5 gssrpc

CLEANFILES+= kadm5 gssrpc

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

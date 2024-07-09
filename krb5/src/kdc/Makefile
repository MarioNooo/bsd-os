BINDIR=	/usr/libexec
PROG=	krb5kdc
MAN8=	krb5kdc.0

.depend: krb5kdc.8 kdc5_err.c kdc5_err.h

CLEANFILES+= krb5kdc.8 kdc5_err.c kdc5_err.h

CFLAGS+=-I. -I${DESTDIR}/usr/include/krb5 \
	-I${.CURDIR}/../include/kerberosIV -I${.CURDIR}/../include/krb5 \
	-I${.CURDIR}/../include -I${.CURDIR}/../include/obj \
	-I${.CURDIR}/../util/et \
	-I${.CURDIR}/../util/profile -I${.CURDIR}/../util/profile/obj

CFLAGS+=-DPOSIX_TERMIOS=1 -DHAVE_SYSLOG_H=1 -DHAVE_STDARG_H=1 \
	-DHAVE_SYS_SELECT_H=1 -DHAVE_SYS_SOCKIO_H=1 -DHAVE_OPENLOG=1 \
	-DHAVE_SYSLOG=1 -DHAVE_CLOSELOG=1 -DHAVE_STRFTIME=1 \
	-DHAVE_VSPRINTF=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_SOCKET_H=1 \
	-DHAVE_NETINET_IN_H=1 -DHAVE_INET_NTOP=1 -DHAVE_INET_PTON=1 \
	-DHAVE_GETIPNODEBYNAME=1 -DHAVE_GETIPNODEBYADDR=1 \
	-DHAVE_GETADDRINFO=1 -DHAVE_GETNAMEINFO=1 -DKRB5_USE_INET6=1 \
	-DHAVE_SA_LEN=1 -DPOSIX_SIGNALS=1 -DUSE_RCACHE=1 

.PATH:  ${.CURDIR}/../lib/kadm5

SRCS =	dispatch.c do_as_req.c do_tgs_req.c kdc_util.c kdc_preauth.c \
	main.c network.c sock2p.c policy.c extern.c replay.c kerberos_v4.c
SRCS+=	logger.c
SRCS+=	kdc5_err.c
	
# krb5kdc: $(OBJS) $(KADMSRV_LIBS) $(KRB4COMPAT_LIBS)

# KADMSRV_LIBS
LDADD+= -lkadm5srv -lprofile -lkdb5
LDADD+= -L${.CURDIR}/../util/db2 -L${.CURDIR}/../util/db2/obj -ldb
LDADD+= -lgssrpc -ldyn -lgssapi_krb5
# KRB4COMPAT_LIBS
LDADD+=	-lkrb4 -lkrb5 -ldes425 -lk5crypto -lcom_err

krb5kdc.8: ${.CURDIR}/krb5kdc.M
	cp ${.ALLSRC} ${.TARGET}
kdc5_err.h: ${.CURDIR}/kdc5_err.et
	awk -f ${.CURDIR}/../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}
kdc5_err.c: ${.CURDIR}/kdc5_err.et
	awk -f ${.CURDIR}/../util/et/et_c.awk outfile=${.TARGET} ${.ALLSRC}

.include <bsd.prog.mk>

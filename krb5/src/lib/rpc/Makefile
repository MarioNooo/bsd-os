LIB=gssrpc
LDDYNMAJOR=.3
LDDYNMINOR=.0

CLEANFILES+=tags gssrpc gssapi gssapi_generic.h gssapi_krb5.h
.depend: gssrpc gssapi gssapi_generic.h gssapi_krb5.h

SRCS =	auth_none.c auth_unix.c auth_any.c authunix_prot.c auth_gssapi.c \
        auth_gssapi_misc.c bindresvport.c clnt_generic.c clnt_perror.c \
        clnt_raw.c clnt_simple.c clnt_tcp.c clnt_udp.c rpc_dtablesize.c \
        get_myaddress.c getrpcport.c pmap_clnt.c pmap_getmaps.c \
        pmap_getport.c pmap_prot.c pmap_prot2.c pmap_rmt.c rpc_prot.c \
        rpc_commondata.c rpc_callmsg.c svc.c svc_auth.c svc_auth_unix.c \
        svc_auth_any.c svc_auth_gssapi.c svc_raw.c svc_run.c \
        svc_simple.c svc_tcp.c svc_udp.c xdr.c xdr_array.c xdr_float.c \
        xdr_mem.c xdr_rec.c xdr_reference.c xdr_stdio.c xdr_alloc.c

CFLAGS+=-I. -I${.CURDIR} -I${DESTDIR}/usr/include/krb5 \
	-I${.CURDIR}/../gssapi/obj -I${.CURDIR}/../gssapi \
	-I${.CURDIR}/../../util/dyn -I${.CURDIR}/../../util/et \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj \
	-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj
CFLAGS+=-DHAVE_SYS_UIO_H=1 -DSIZEOF_INT=4 -DSIZEOF_LONG=4 \
	-DHAVE_STRERROR=1 -DSETRPCENT_TYPE=void -DENDRPCENT_TYPE=void \
	-DSYS_ERRLIST_DECLARED=1 -DHAVE_SYS_ERRLIST=1 
CFLAGS+=-DGSSAPI_KRB5

# this is so sleazy
gssrpc:
	if [ ! -h gssrpc ]; then \
		ln -s ${.CURDIR} gssrpc; \
	fi

gssapi:
	if [ ! -h gssapi ]; then \
		ln -s . gssapi; \
	fi

gssapi_generic.h:
	if [ ! -h gssapi_generic.h ]; then \
		ln -s ${.CURDIR}/../gssapi/generic/gssapi_generic.h ; \
	fi

gssapi_krb5.h:
	if [ ! -h gssapi_krb5.h ]; then \
		ln -s ${.CURDIR}/../gssapi/krb5/gssapi_krb5.h ; \
	fi

.include <bsd.lib.mk>

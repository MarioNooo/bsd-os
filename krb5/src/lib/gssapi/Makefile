LIB=gssapi_krb5
LDDYNMAJOR=.2
LDDYNMINOR=.2

CLEANFILES+=tags gssapi
CLEANFILES+=gssapi_err_generic.h gssapi_err_generic.c \
	gssapi_err_krb5.h gssapi_err_krb5.c gssapi.h

.depend: gssapi_err_generic.h gssapi_err_generic.c \
	gssapi_err_krb5.h gssapi_err_krb5.c gssapi.h gssapi

.PATH:	${.CURDIR}/generic ${.CURDIR}/krb5

CFLAGS+= -I. -I${.CURDIR} -I${DESTDIR}/usr/include/krb5 \
	-I${.CURDIR}/../../util/et -I${.CURDIR}/../../include/krb5 \
	-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj \
	-I${.CURDIR}/generic -I${.CURDIR}/krb5

CFLAGS+=-DHAVE_STDLIB_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_LIMITS_H=1 \
	-DSTDC_HEADERS=1 -DSIZEOF_SHORT=2 -DSIZEOF_INT=4 -DSIZEOF_LONG=4

# sources in various subdirectories
SRCS =	disp_com_err_status.c disp_major_status.c gssapi_generic.c \
	oid_ops.c rel_buffer.c rel_oid_set.c util_buffer.c \
	util_canonhost.c util_dup.c util_oid.c util_ordering.c \
	util_set.c util_token.c util_validate.c gssapi_err_generic.c
SRCS+=	accept_sec_context.c acquire_cred.c add_cred.c canon_name.c \
	compare_name.c context_time.c copy_ccache.c delete_sec_context.c \
	disp_name.c disp_status.c duplicate_name.c export_name.c \
	export_sec_context.c get_tkt_flags.c gssapi_krb5.c \
	import_name.c import_sec_context.c indicate_mechs.c \
	init_sec_context.c inq_context.c inq_cred.c inq_names.c \
	k5seal.c k5unseal.c krb5_gss_glue.c process_context_token.c \
	rel_cred.c rel_oid.c rel_name.c seal.c ser_sctx.c set_ccache.c \
	sign.c unseal.c util_cksum.c util_crypt.c util_ctxsetup.c \
	util_seed.c util_seqnum.c val_cred.c verify.c wrap_size_limit.c \
	gssapi_err_krb5.c
# unused for some reason
# SRCS+=	k5mech.c pname_to_uid.c
# sources in this directory
SRCS+=	gss_libinit.c

gssapi_err_generic.h: ${.CURDIR}/generic/gssapi_err_generic.et
	awk -f ${.CURDIR}/../../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}
gssapi_err_generic.c: ${.CURDIR}/generic/gssapi_err_generic.et
	awk -f ${.CURDIR}/../../util/et/et_c.awk outfile=${.TARGET} ${.ALLSRC}
gssapi_err_krb5.h: ${.CURDIR}/krb5/gssapi_err_krb5.et
	awk -f ${.CURDIR}/../../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}
gssapi_err_krb5.c: ${.CURDIR}/krb5/gssapi_err_krb5.et
	awk -f ${.CURDIR}/../../util/et/et_c.awk outfile=${.TARGET} ${.ALLSRC}

gssapi:
	if [ ! -h gssapi ]; then \
		ln -s . gssapi; \
	fi

gssapi.h: ${.CURDIR}/generic/gssapi.hin
	echo "/* This is the gssapi.h prologue. */" > ${.TARGET}
	echo "/* It contains some choice pieces of autoconf.h */" >> ${.TARGET}
	grep SIZEOF ${.CURDIR}/../../include/krb5/autoconf.h >> ${.TARGET}
	grep 'HAVE_.*_H' ${.CURDIR}/../../include/krb5/autoconf.h >> ${.TARGET}
	grep 'USE_.*_H' ${.CURDIR}/../../include/krb5/autoconf.h >> ${.TARGET}
	echo "/* End of gssapi.h prologue. */" >> ${.TARGET}
	cat ${.ALLSRC} >> ${.TARGET}

beforeinstall: gssapi.h ${.CURDIR}/generic/gssapi_generic.h \
	${.CURDIR}/krb5/gssapi_krb5.h
	if [ ! -d ${DESTDIR}${KRB5_INCDIR} ]; then \
		mkdir ${DESTDIR}${KRB5_INCDIR}; \
	fi
	if [ ! -d ${DESTDIR}${KRB5_INCDIR}/gssapi ]; then \
		mkdir ${DESTDIR}${KRB5_INCDIR}/gssapi; \
	fi
	install -c -m 444 gssapi.h ${DESTDIR}${KRB5_INCDIR}/gssapi
	install -c -m 444 ${.CURDIR}/generic/gssapi_generic.h \
		${DESTDIR}${KRB5_INCDIR}/gssapi
	install -c -m 444 ${.CURDIR}/krb5/gssapi_krb5.h \
		${DESTDIR}${KRB5_INCDIR}/gssapi

.include <bsd.lib.mk>

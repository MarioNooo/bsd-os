LIB=krb4
LDDYNMAJOR=.2
LDDYNMINOR=.0

CLEANFILES+= tags krb_err.c krb_err.h
.depend: krb_err.c krb_err.h

SRCS =	cr_auth_repl.c cr_ciph.c cr_tkt.c debug.c decomp_tkt.c g_ad_tkt.c \
	g_pw_in_tkt.c g_phost.c g_pw_tkt.c g_tkt_svc.c getst.c \
	gethostname.c kname_parse.c err_txt.c g_in_tkt.c mk_auth.c \
	mk_err.c mk_priv.c mk_req.c mk_safe.c month_sname.c one.c \
	pkt_cipher.c pkt_clen.c rd_err.c rd_priv.c rd_safe.c \
	send_to_kdc.c stime.c rd_preauth.c mk_preauth.c
SRCS+=	unix_time.c
SRCS+=	tf_util.c dest_tkt.c in_tkt.c tkt_string.c g_tf_fname.c \
	g_tf_realm.c g_cred.c save_creds.c
SRCS+=	klog.c kuserok.c log.c kntoln.c fgetst.c rd_svc_key.c \
	cr_err_repl.c rd_req.c g_svc_in_tkt.c recvauth.c \
	ad_print.c cr_death_pkt.c kparse.c put_svc_key.c sendauth.c 
SRCS+=	netread.c netwrite.c
SRCS+=	g_cnffile.c g_krbhst.c g_krbrlm.c g_admhst.c realmofhost.c
SRCS+=	unix_glue.c
# this one is auto-generated
SRCS+=	krb_err.c

CFLAGS+= -I. -I${.CURDIR} -I${DESTDIR}/usr/include/krb5 \
	-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/../../include/krb5 \
	-I${.CURDIR}/../../util/et \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj \
	-I${.CURDIR}/../../include/kerberosIV
CFLAGS+=-DHAVE_SYS_SELECT_H=1 -DHAVE_TIME_H=1 -DHAVE_UNISTD_H=1 \
	-DHAVE_STDLIB_H=1 -DLSBFIRST=1 -DSIZEOF_INT=4 -DBITS32=1 \
	-DKRB4_USE_KEYTAB=1 -DHAVE_STRDUP=1 -DHAVE_SETEUID=1 -DHAVE_SETREUID=1 


krb_err.h: ${.CURDIR}/krb_err.et
	awk -f ${.CURDIR}/../../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}
krb_err.c: ${.CURDIR}/krb_err.et
	awk -f ${.CURDIR}/../../util/et/et_c.awk outfile=${.TARGET} ${.ALLSRC}

.include <bsd.lib.mk>

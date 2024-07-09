LIB=krb5
LDDYNMAJOR=.3
LDDYNMINOR=.1

CLEANFILES+=tags
CLEANFILES+=asn1_err.c kdb5_err.c krb5_err.c kv5m_err.c adm_err.c \
	asn1_err.h kdb5_err.h krb5_err.h kv5m_err.h adm_err.h

.depend:asn1_err.c kdb5_err.c krb5_err.c kv5m_err.c adm_err.c \
	asn1_err.h kdb5_err.h krb5_err.h kv5m_err.h adm_err.h

.PATH:	${.CURDIR}/error_tables ${.CURDIR}/asn.1 \
	${.CURDIR}/ccache ${.CURDIR}/ccache/stdio \
	${.CURDIR}/ccache/file ${.CURDIR}/ccache/memory \
	${.CURDIR}/keytab ${.CURDIR}/keytab/file \
	${.CURDIR}/keytab/srvtab ${.CURDIR}/krb ${.CURDIR}/rcache \
	${.CURDIR}/os ${.CURDIR}/posix

CFLAGS+= -I. -I${.CURDIR} -I${DESTDIR}/usr/include/krb5 \
	-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/../../include/krb5 -I${.CURDIR}/../../util/et \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj \
	-I${.CURDIR}/error_tables -I${.CURDIR}/asn.1 \
	-I${.CURDIR}/ccache -I${.CURDIR}/ccache/stdio \
	-I${.CURDIR}/ccache/file -I${.CURDIR}/ccache/memory \
	-I${.CURDIR}/keytab -I${.CURDIR}/keytab/file \
	-I${.CURDIR}/keytab/srvtab -I${.CURDIR}/krb -I${.CURDIR}/rcache \
	-I${.CURDIR}/os -I${.CURDIR}/posix

CFLAGS+= -DHAVE_STDARG_H=1 -DHAVE_UNISTD_H=1 -DHAVE_PATHS_H=1 \
	-DHAVE_REGEX_H=1 -DHAVE_REGEXP_H=1 -DHAVE_FCNTL_H=1 \
	-DHAVE_FLOCK=1 -DHAVE_FCHMOD=1 -DHAVE_CHMOD=1 \
	-DHAVE_STRFTIME=1 -DHAVE_GETEUID=1 -DHAVE_SETENV=1 \
	-DHAVE_UNSETENV=1 -DHAVE_GETENV=1 -DHAVE_SETSID=1 \
	-DHAVE_GETHOSTBYNAME2=1 -DHAVE_VFPRINTF=1 -DHAVE_VSPRINTF=1 \
	-DHAVE_STRDUP=1 -DHAVE_STRCASECMP=1 -DHAVE_STRERROR=1 \
	-DHAVE_MEMMOVE=1 -DHAVE_DAEMON=1 -DHAVE_GETUID=1 \
	-DHAVE_SSCANF=1 -DHAVE_SYSLOG=1 -DHAVE_REGEXEC=1 \
	-DHAVE_REGCOMP=1 -DHAVE_SA_LEN=1 

# sources in various subdirectories
SRCS =	init_ets.c
SRCS+=	asn1_decode.c asn1_k_decode.c asn1_encode.c asn1_get.c \
	asn1_make.c asn1buf.c krb5_decode.c krb5_encode.c \
	asn1_k_encode.c asn1_misc.c
SRCS+=	ccbase.c cccopy.c ccdefault.c ccdefops.c cc_retr.c ser_cc.c
SRCS+= 	scc_close.c scc_destry.c scc_eseq.c scc_gennew.c scc_getnam.c \
	scc_gprin.c scc_init.c scc_nseq.c scc_read.c scc_reslv.c \
	scc_retrv.c scc_sseq.c scc_store.c scc_skip.c scc_ops.c scc_write.c \
	scc_sflags.c scc_defops.c scc_errs.c scc_maybe.c
SRCS+=	fcc_close.c fcc_destry.c fcc_eseq.c fcc_gennew.c fcc_getnam.c \
	fcc_gprin.c fcc_init.c fcc_nseq.c fcc_read.c fcc_reslv.c \
	fcc_retrv.c fcc_sseq.c fcc_store.c fcc_skip.c fcc_ops.c \
	fcc_write.c fcc_sflags.c fcc_defops.c fcc_errs.c fcc_maybe.c
SRCS+=	mcc_close.c mcc_destry.c mcc_eseq.c mcc_gennew.c mcc_getnam.c \
	mcc_gprin.c mcc_init.c mcc_nseq.c mcc_reslv.c mcc_retrv.c \
	mcc_sseq.c mcc_store.c mcc_ops.c mcc_sflags.c
SRCS+=	ktadd.c ktbase.c ktdefault.c ktfr_entry.c ktremove.c read_servi.c
SRCS+=	ktf_add.c ktf_close.c ktf_endget.c ktf_g_ent.c ktf_g_name.c \
	ktf_next.c ktf_resolv.c ktf_remove.c ktf_ssget.c ktf_util.c \
	ktf_ops.c ktf_wops.c ktf_wreslv.c ktf_defops.c ser_ktf.c
SRCS+=	kts_close.c kts_endget.c kts_g_ent.c kts_g_name.c kts_next.c \
	kts_ops.c kts_resolv.c kts_ssget.c kts_util.c
SRCS+=	addr_comp.c addr_order.c addr_srch.c appdefault.c auth_con.c \
	bld_pr_ext.c bld_princ.c chk_trans.c chpw.c conv_princ.c \
	copy_addrs.c copy_auth.c copy_athctr.c copy_cksum.c copy_creds.c \
	copy_data.c copy_key.c copy_princ.c copy_tick.c cp_key_cnt.c \
	decode_kdc.c decrypt_tk.c deltat.c enc_helper.c encode_kdc.c \
	encrypt_tk.c free_rtree.c fwd_tgt.c gc_frm_kdc.c gc_via_tkt.c \
	gen_seqnum.c gen_subkey.c get_creds.c get_in_tkt.c gic_keytab.c \
	gic_opt.c gic_pwd.c in_tkt_ktb.c in_tkt_pwd.c in_tkt_sky.c \
	init_ctx.c init_keyblock.c kdc_rep_dc.c kfree.c mk_cred.c \
	mk_error.c mk_priv.c mk_rep.c mk_req.c mk_req_ext.c mk_safe.c \
	parse.c pr_to_salt.c preauth.c preauth2.c princ_comp.c rd_cred.c \
	rd_error.c rd_priv.c rd_rep.c rd_req.c rd_req_dec.c rd_safe.c \
	recvauth.c sendauth.c send_tgs.c ser_actx.c ser_adata.c \
	ser_addr.c ser_auth.c ser_cksum.c ser_ctx.c ser_eblk.c ser_key.c \
	ser_princ.c serialize.c set_realm.c srv_rcache.c str_conv.c \
	tgtname.c unparse.c valid_times.c vfy_increds.c vic_opt.c \
	walk_rtree.c
SRCS+=	rc_base.c rc_dfl.c rc_io.c rcdef.c rc_conv.c ser_rc.c
SRCS+=	an_to_ln.c c_ustime.c def_realm.c ccdefname.c changepw.c \
        free_krbhs.c free_hstrl.c full_ipadr.c get_krbhst.c gen_port.c \
        genaddrs.c gen_rname.c gmt_mktime.c hostaddr.c hst_realm.c \
        init_os_ctx.c krbfileio.c ktdefname.c kuserok.c mk_faddr.c \
        localaddr.c locate_kdc.c lock_file.c net_read.c net_write.c \
        osconfig.c port2ip.c prompter.c promptusr.c read_msg.c \
        read_pwd.c realm_dom.c realm_iter.c sendto_kdc.c sn2princ.c \
        timeofday.c toffset.c unlck_file.c ustime.c write_msg.c
SRCS+=	setenv.c
# sources in this directory
SRCS+=	krb5_libinit.c
# sources that are auto-generated
SRCS+=	asn1_err.c kdb5_err.c krb5_err.c kv5m_err.c adm_err.c

asn1_err.h: ${.CURDIR}/error_tables/asn1_err.et
	awk -f ${.CURDIR}/../../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}
kdb5_err.h: ${.CURDIR}/error_tables/kdb5_err.et
	awk -f ${.CURDIR}/../../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}
krb5_err.h: ${.CURDIR}/error_tables/krb5_err.et
	awk -f ${.CURDIR}/../../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}
adm_err.h: ${.CURDIR}/error_tables/adm_err.et
	awk -f ${.CURDIR}/../../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}
kv5m_err.h: ${.CURDIR}/error_tables/kv5m_err.et
	awk -f ${.CURDIR}/../../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}

asn1_err.c: ${.CURDIR}/error_tables/asn1_err.et
	awk -f ${.CURDIR}/../../util/et/et_c.awk outfile=${.TARGET} ${.ALLSRC}
kdb5_err.c: ${.CURDIR}/error_tables/kdb5_err.et
	awk -f ${.CURDIR}/../../util/et/et_c.awk outfile=${.TARGET} ${.ALLSRC}
krb5_err.c: ${.CURDIR}/error_tables/krb5_err.et
	awk -f ${.CURDIR}/../../util/et/et_c.awk outfile=${.TARGET} ${.ALLSRC}
adm_err.c: ${.CURDIR}/error_tables/adm_err.et
	awk -f ${.CURDIR}/../../util/et/et_c.awk outfile=${.TARGET} ${.ALLSRC}
kv5m_err.c: ${.CURDIR}/error_tables/kv5m_err.et
	awk -f ${.CURDIR}/../../util/et/et_c.awk outfile=${.TARGET} ${.ALLSRC}

.include <bsd.lib.mk>

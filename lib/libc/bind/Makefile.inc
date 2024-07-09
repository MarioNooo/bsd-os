# BSDI Makefile.inc,v 1.17 2003/06/18 16:43:55 polk Exp

BINDDIR=${.CURDIR}/../../core_contrib/bind/lib/bind
BINDLIBSRC=${BINDDIR}
BINDINCSRC=${BINDDIR}/include
BINDMAN3=	getaddrinfo.0 gethostbyname.0 getipnodebyname.0 getnameinfo.0 \
		getnetent.0 hesiod.0 inet_cidr.0 resolver.0 tsig.0

CFLAGS+=-I${BINDINCSRC} -I${BINDLIBSRC} -I${BINDDIR}/port/bsdos/include
CFLAGS+=-D_REENTRANT= 
CFLAGS+=-DHAVE_PORTABLE_PROTOTYPE
CFLAGS+=-DHAVE_SOCKLEN_T
CFLAGS+=-DINET6

.PATH:	${.CURDIR}/bind ${BINDLIBSRC}/dst \
	${BINDLIBSRC}/inet ${BINDLIBSRC}/irs ${BINDLIBSRC}/isc \
	${BINDLIBSRC}/nameser ${BINDLIBSRC}/resolv

bindbeforeinstall: 
	cd ${BINDINCSRC} && install -c \
	    -o ${BINOWN} -g ${BINGRP} -m 444 hesiod.h irp.h irs.h \
	    netdb.h netgroup.h res_update.h resolv.h ${DESTDIR}/usr/include/
	cd ${BINDINCSRC}/arpa && install -c \
	    -o ${BINOWN} -g ${BINGRP} -m 444 inet.h nameser.h \
	    nameser_compat.h ${DESTDIR}/usr/include/arpa/
	cd ${BINDINCSRC}/isc && install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    assertions.h dst.h eventlib.h heap.h irpmarshall.h list.h \
	    logging.h memcluster.h misc.h tree.h ${DESTDIR}/usr/include/isc/
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${BINDDIR}/port/bsdos/include/sys/bitypes.h \
	    ${DESTDIR}/usr/include/sys/

## dst:
SRCS+=	dst_api.c hmac_link.c md5_dgst.c support.c

# inet
SRCS+=	inet_addr.c inet_cidr_ntop.c inet_cidr_pton.c inet_lnaof.c \
	inet_makeaddr.c inet_net_ntop.c inet_net_pton.c inet_neta.c \
	inet_netof.c inet_network.c inet_ntoa.c inet_ntop.c inet_pton.c \
	nsap_addr.c

## isc
SRCS+=	assertions.c base64.c bitncmp.c ctl_clnt.c ctl_p.c ctl_srvr.c \
	ev_connects.c ev_files.c ev_streams.c ev_timers.c ev_waits.c \
	eventlib.c heap.c hex.c logging.c memcluster.c movefile.c tree.c

# irs
SRCS+=	dns.c dns_gr.c dns_ho.c dns_nw.c dns_pr.c dns_pw.c dns_sv.c \
	gen.c gen_gr.c gen_ho.c gen_ng.c gen_nw.c gen_pr.c \
	gen_pw.c gen_sv.c getaddrinfo.c getgrent.c getgrent_r.c gethostent.c \
	gethostent_r.c getnameinfo.c getnetent.c getnetent_r.c getnetgrent.c \
	getnetgrent_r.c getprotoent.c getprotoent_r.c getpwent.c \
	getpwent_r.c getservent.c getservent_r.c hesiod.c irp.c irp_gr.c \
	irp_ho.c irp_ng.c irp_nw.c irp_pr.c irp_pw.c irp_sv.c irpmarshall.c \
	irs_data.c lcl.c lcl_gr.c lcl_ho.c lcl_ng.c lcl_nw.c lcl_pr.c \
	lcl_pw.c lcl_sv.c nis.c nis_gr.c nis_ho.c nis_ng.c nis_nw.c \
	nis_pr.c nis_pw.c nis_sv.c nul_ng.c util.c

## nameser
SRCS+=	ns_date.c ns_name.c ns_netint.c ns_parse.c ns_print.c \
	ns_samedomain.c ns_sign.c ns_ttl.c ns_verify.c

# resolv
SRCS+=	herror.c res_comp.c res_data.c res_debug.c res_findzonecut.c \
	res_init.c res_mkquery.c res_mkupdate.c res_query.c res_send.c \
	res_sendsigned.c res_update.c

MAN3+=	${BINDMAN3}

MLINKS+= getaddrinfo.3 freeaddrinfo.3 getaddrinfo.3 gai_strerror.3

MLINKS+=gethostbyname.3 gethostbyname2.3 gethostbyname.3 gethostbyaddr.3 \
	gethostbyname.3 gethostent.3 gethostbyname.3 sethostent.3 \
	gethostbyname.3 endhostent.3 gethostbyname.3 herror.3

MLINKS+=getipnodebyname.3 getipnodebyaddr.3 getipnodebyname.3 freehostent.3

MLINKS+=getnetent.3 getnetbyaddr.3 getnetent.3 getnetbyname.3 \
	getnetent.3 setnetent.3 getnetent.3 endnetent.3

MLINKS+=inet_cidr.3 inet_cidr_ntop.3 inet_cidr.3 inet_cidr_pton.3

MLINKS+=hesiod.3 hesiod_init.3 hesiod.3 hesiod_resolv.3 \
	hesiod.3 hesiod_free_list.3 hesiod.3 hesiod_to_bind.3 \
	hesiod.3 hesiod_end.3

MLINKS+=resolver.3 res_ninit.3 resolver.3 res_nisourserver.3 \
	resolver.3 fp_resstat.3 resolver.3 res_nquery.3 \
	resolver.3 res_nsearch.3 resolver.3 res_nquerydomain.3 \
	resolver.3 res_nmkquery.3 resolver.3 res_nsend.3 \
	resolver.3 res_nupdate.3 resolver.3 res_nmkupdate.3 \
	resolver.3 res_nclose.3 resolver.3 res_nsendsigned.3 \
	resolver.3 res_nfindprimary.3 resolver.3 res_nsendupdate.3 \
	resolver.3 dn_comp.3 resolver.3 dn_expand.3 resolver.3 hstrerror.3 \
	resolver.3 res_init.3 resolver.3 res_isourserver.3 \
	resolver.3 p_nquery.3 resolver.3 res_query.3 resolver.3 res_search.3 \
	resolver.3 res_querydomain.3 resolver.3 res_mkquery.3 \
	resolver.3 res_send.3 resolver.3 res_update.3 resolver.3 res_close.3

MLINKS+=tsig.3 ns_sign.3 tsig.3 ns_sign_tcp.3 tsig.3 ns_sign_tcp_init.3 \
	tsig.3 ns_verify.3 tsig.3 ns_verify_tcp.3 tsig.3 ns_verify_tcp_init.3 \
	tsig.3 ns_find_tsig.3

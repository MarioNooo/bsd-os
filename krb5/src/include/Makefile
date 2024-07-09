CLEANFILES+=krb5.h krb_err.h osconf.h \
	krb5_err.h kdb5_err.h kv5m_err.h asn1_err.h adm_err.h

.depend: krb5.h osconf.h krb_err.h adm_err.h

#krb5/autoconf.h:
#	(cd krb5; $(MAKE) autoconf.h)

krb5.h: ${.CURDIR}/krb5/autoconf.h ${.CURDIR}/krb5.hin krb5_err.h \
	kdb5_err.h kv5m_err.h asn1_err.h
	echo "/* This is the prologue to krb5.h */" > krb5.h
	echo "/* Unfortunately some of these defines are compiler dependent */" >> krb5.h
	grep SIZEOF ${.CURDIR}/krb5/autoconf.h >> krb5.h
	grep HAVE_STDARG_H ${.CURDIR}/krb5/autoconf.h >> krb5.h
	grep HAVE_SYS_TYPES_H ${.CURDIR}/krb5/autoconf.h >> krb5.h
	echo "/* End of prologue section */"  >> krb5.h
	cat ${.CURDIR}/krb5.hin krb5_err.h kdb5_err.h kv5m_err.h \
		asn1_err.h >> krb5.h

osconf.h: ${.CURDIR}/krb5/stock/osconf.h
	sed -e "s+@KRB5RCTMPDIR+/tmp+" \
		-e "s+@PREFIX+/usr+" \
		-e "s+@EXEC_PREFIX+/usr+" \
		-e 's+@LOCALSTATEDIR+/etc/krb5+' \
		-e 's+@SYSCONFDIR+/etc/krb5+' ${.ALLSRC} > ${.TARGET}

#
# Build the error table include files:
# adm_err.h asn1_err.h kdb5_err.h krb5_err.h kv5m_err.h

# using ${.ALLSRC} isn't quite correct, but it does work.

adm_err.h: ${.CURDIR}/../lib/krb5/error_tables/adm_err.et
	awk -f ${.CURDIR}/../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}

asn1_err.h: ${.CURDIR}/../lib/krb5/error_tables/asn1_err.et
	awk -f ${.CURDIR}/../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}

kdb5_err.h: ${.CURDIR}/../lib/krb5/error_tables/kdb5_err.et
	awk -f ${.CURDIR}/../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}

krb5_err.h: ${.CURDIR}/../lib/krb5/error_tables/krb5_err.et
	awk -f ${.CURDIR}/../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}

kv5m_err.h: ${.CURDIR}/../lib/krb5/error_tables/kv5m_err.et
	awk -f ${.CURDIR}/../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}

krb_err.h: ${.CURDIR}/../lib/krb4/krb_err.et
	awk -f ${.CURDIR}/../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}

install: krb5.h osconf.h krb_err.h ${.CURDIR}/krb5/autoconf.h
	if [ ! -d ${DESTDIR}${KRB5_INCDIR} ]; then \
		mkdir ${DESTDIR}${KRB5_INCDIR}; \
	fi
	if [ ! -d ${DESTDIR}${KRB5_INCDIR}/kerberosIV ]; then \
		mkdir ${DESTDIR}${KRB5_INCDIR}/kerberosIV; \
	fi
	install -c -m 444 krb5.h ${DESTDIR}${KRB5_INCDIR}
	install -c -m 444 osconf.h ${DESTDIR}${KRB5_INCDIR}
	install -c -m 444 ${.CURDIR}/port-sockets.h ${DESTDIR}${KRB5_INCDIR}
	install -c -m 444 ${.CURDIR}/k5-util.h ${DESTDIR}${KRB5_INCDIR}
	install -c -m 444 ${.CURDIR}/k5-int.h ${DESTDIR}${KRB5_INCDIR}
	install -c -m 444 ${.CURDIR}/krb5/adm.h ${DESTDIR}${KRB5_INCDIR}
	install -c -m 444 ${.CURDIR}/krb5/adm_proto.h ${DESTDIR}${KRB5_INCDIR}
	install -c -m 444 ${.CURDIR}/krb5/kdb.h ${DESTDIR}${KRB5_INCDIR}
	install -c -m 444 ${.CURDIR}/krb5/kdb_dbm.h ${DESTDIR}${KRB5_INCDIR}
	install -c -m 444 ${.CURDIR}/krb5/autoconf.h ${DESTDIR}${KRB5_INCDIR}
	install -c -m 444 ${.CURDIR}/kerberosIV/krb.h ${DESTDIR}${KRB5_INCDIR}/kerberosIV
	install -c -m 444 ${.CURDIR}/kerberosIV/des.h ${DESTDIR}${KRB5_INCDIR}/kerberosIV
	install -c -m 444 ${.CURDIR}/kerberosIV/kadm.h ${DESTDIR}${KRB5_INCDIR}/kerberosIV
	install -c -m 444 ${.CURDIR}/kerberosIV/kparse.h ${DESTDIR}${KRB5_INCDIR}/kerberosIV
	install -c -m 444 ${.CURDIR}/kerberosIV/mit-copyright.h ${DESTDIR}${KRB5_INCDIR}/kerberosIV
	install -c -m 444 krb_err.h ${DESTDIR}${KRB5_INCDIR}/kerberosIV

.include <bsd.prog.mk>

LIB=ss
CLEANFILES+=tags mk_cmds ct_c.awk ct_c.sed ss_err.h ss_err.c std_rqs.c ss

LDDYNMAJOR=.1
LDDYNMINOR=.0

CFLAGS+= -I${.CURDIR}/../et -I${.CURDIR}/.. -I${.CURDIR} -I.

SRCS=	invocation.c help.c execute_cmd.c listen.c \
	parse.c error.c prompt.c request_tbl.c list_rqs.c pager.c \
	requests.c data.c std_rqs.c ss_err.c

.depend: std_rqs.c ss_err.h ss_err.c

ss_err.h: ss_err.et
	awk -f ${.CURDIR}/../et/et_h.awk outfile=${.TARGET} ${.CURDIR}/ss_err.et
ss_err.c: ss_err.et
	awk -f ${.CURDIR}/../et/et_c.awk outfile=${.TARGET} ${.CURDIR}/ss_err.et

std_rqs.c: mk_cmds.sh
	if [ ! -d ${DESTDIR}/usr/share/krb5 ]; then \
		mkdir ${DESTDIR}/usr/share/krb5; \
	fi
	sed -e "s;@DIR@;/usr/share/krb5;" -e "s;@AWK@;/usr/bin/awk;" \
		-e "s;@SED@;/usr/bin/sed;" \
		${.CURDIR}/mk_cmds.sh > mk_cmds && \
	chmod 755 mk_cmds && \
	cp ${.CURDIR}/ct_c_awk.in ct_c.awk && \
	sed -e '/^#/d' ${.CURDIR}/ct_c_sed.in > ct_c.sed && \
	install -c -m 755 mk_cmds ${DESTDIR}/usr/share/krb5 && \
	install -c -m 444 ct_c.awk ${DESTDIR}/usr/share/krb5 && \
	install -c -m 444 ct_c.sed ${DESTDIR}/usr/share/krb5 && \
	./mk_cmds ${.CURDIR}/std_rqs.ct && \
	if [ -h ss ]; then \
		rm -f ss; \
	fi; \
	ln -s . ss  

.include <bsd.lib.mk>

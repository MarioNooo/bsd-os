LIB=com_err
CLEANFILES+=tags compile_et

LDDYNMAJOR=.3
LDDYNMINOR=.0

SRCS=	error_message.c et_name.c init_et.c com_err.c

beforeinstall:
	if [ ! -d ${DESTDIR}/usr/include/krb5 ]; then \
		mkdir ${DESTDIR}/usr/include/krb5; \
	fi
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${.CURDIR}/com_err.h ${DESTDIR}/usr/include/krb5
	@(if [ ! -d ${DESTDIR}/usr/share/krb5 ]; then \
	mkdir -p ${DESTDIR}/usr/share/krb5; \
	fi ) && \
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${.CURDIR}/et_c.awk ${.CURDIR}/et_h.awk \
		${DESTDIR}/usr/share/krb5 &&
	sed -e "s~@AWK@~/usr/bin/awk~" \
		-e "s~@DIR@~/usr/share/krb5~" \
		${.CURDIR}/compile_et.sh > compile_et &&
	install -c -o ${BINOWN} -g ${BINGRP} -m 755 \
		compile_et ${DESTDIR}/usr/sbin

.include <bsd.lib.mk>

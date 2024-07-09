POPUID=root
LIBEXECDIR=/usr/libexec
SBINDIR=/usr/sbin
MANDIR=/usr/share/core_contrib/man/cat
MANSRCDIR=/usr/share/core_contrib/man/man
SBINPROGS=popper/popauth
LIBEXECPROGS=popper/popper
MAN8!=ls man/*.8 | grep -v poppassd | sed 's/.8/.0/'
CLEANFILES=${MAN8}

.if exists(${.CURDIR}/../../domestic/Makefile)
KERBEROS_LIBS=-lkrb5util -lkrb4 -ldes425 -lkrb5 -lk5crypto -lprofile -lcom_err
KERBEROS_OPT=--enable-kerberos5 --enable-ksockinst --enable-kuserok
CFLAGS+=-I/usr/include/krb5
.else
KERBEROS_LIBS=
KERBEROS_OPT=
.endif

.if exists(/usr/include/openssl)
OPENSSLROOT=/usr
.else
OPENSSLROOT=/usr/contrib
.endif

all:	realall manpages

realall:	Makefile
	make -f Makefile

clean:
	if [ -f Makefile ]; then \
		make -f Makefile clean; \
	fi
	rm -rf ${CLEANFILES}

cleandir:
	if [ -f Makefile ]; then \
		make -f Makefile realclean; \
	fi
	rm -rf ${CLEANFILES}

beforeinstall:
	install ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${LIBEXECPROGS} ${DESTDIR}${LIBEXECDIR}
	install ${COPY} ${STRIP} -o ${POPUID} -g ${BINGRP} -m 4555 \
		${SBINPROGS} ${DESTDIR}${SBINDIR}

depend:	Makefile

Makefile:
	CC="${LDCC}" CFLAGS="${CFLAGS}" KERBEROS_LIBS="${KERBEROS_LIBS}" \
		./configure -q \
			--enable-warnings \
			--enable-specialauth \
			--enable-apop=/etc/pop.auth --enable-popuid=${POPUID} \
			--enable-uw-kludge \
			${KERBEROS_OPT} \
			--with-openssl=${OPENSSLROOT}
	echo "#define	 RPOP	1" >> config.h
	if [ -n "${KERBEROS_LIBS}" ]; then \
		echo "#define	KERBEROS	1" >> config.h; \
		echo "#define	KRB5	1" >> config.h; \
	fi

obj objdir tags:

.include <bsd.prog.mk>


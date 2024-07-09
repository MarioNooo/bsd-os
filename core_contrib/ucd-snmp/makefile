MAN1!=ls man/*.1 man/*.8.def | sed s/\\.def// | sort -u | sed s/.1/.0/
MAN3!=ls man/*.3 man/*.3.def | sed s/\\.def// | sort -u | sed s/.3/.0/
MAN5!=ls man/*.5 man/*.5.def | sed s/\\.def// | sort -u | sed s/.5/.0/
MAN8!=ls man/*.8 man/*.8.def | sed s/\\.def// | sort -u | sed s/.8/.0/
CLEANFILES+=${MANALL} man1 man3
ETCDIR=/etc
INCDIR=${BINDIR:S%/bin$%/include%}
LIBDIR=${BINDIR:S%/bin$%/lib%}
SHAREDIR=${BINDIR:S%/bin$%/share%}
SBINDIR=${BINDIR:S%/bin$%/sbin%}
LIBINSTALL=install ${COPY} -m 444 -o ${BINOWN} -g ${BINGRP}
BININSTALL=install ${COPY} ${STRIP} -m ${BINMODE} -o ${BINOWN} -g ${BINGRP}
SCRIPTINSTALL=install ${COPY} -m ${BINMODE} -o ${BINOWN} -g ${BINGRP}

MKDIRS=	${ETCDIR}/snmp \
	${INCDIR}/ucd-snmp \
	${SHAREDIR}/snmp \
	${SHAREDIR}/snmp/mibs \
	/var/ucd-snmp

all:	realall manpages

manpages: manpagelinks

manpagelinks:
	ln -sf man man1
	ln -sf man man3

realall:	Makefile
	make -f Makefile
	make manpages

clean:	
	if [ -f Makefile ]; then \
		make -f Makefile clean; \
	fi
	if [ -f perl/SNMP/Makefile ]; then \
		cd perl/SNMP && make -f Makefile clean; \
	fi
	rm -f ${CLEANFILES}

cleandir:	
	if [ -f Makefile ]; then \
		make -f Makefile distclean; \
	fi
	if [ -f perl/SNMP/Makefile ]; then \
		cd perl/SNMP && make -f Makefile distclean; \
	fi
	rm -f ${CLEANFILES}

beforeinstall:
	for dir in ${MKDIRS}; do \
		if [ ! -d ${DESTDIR}$$dir ]; then \
			mkdir -m 755 ${DESTDIR}$$dir; \
		fi \
	done
	make -f Makefile install
	${LIBINSTALL} EXAMPLE.conf ${ETCDIR}/snmp/snmpd.conf-example
	${LIBINSTALL} snmpd.conf-sample ${ETCDIR}/snmp/snmpd.conf-sample
	cd perl/SNMP; \
		perl Makefile.PL --net-snmp-path=/usr && make && make install
	if [ -f perl/SNMP/Makefile ]; then \
		cd perl/SNMP && make -f Makefile distclean; \
	fi

depend:	Makefile

obj objdir:

Makefile:
	PERLPROG=/usr/bin/perl \
		./configure -q \
			--prefix=/usr \
			--mandir=/usr/share/core_contrib/man \
			--sysconfdir=/etc \
			--enable-shared \
			--with-mib-modules="host smux" \
			--with-defaults --with-sys_contact=root

.include <bsd.prog.mk>

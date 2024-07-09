OPTIONAL_DEFINES=	-DINET6
LIBDIR=${BINDIR:S%/bin%/lib%}

MK=${MAKE} -f Makefile CC="${CC}" CCOPT="${CFLAGS} ${OPTIONAL_DEFINES}"

all:	Makefile
	@${MK} 

Makefile:	bpf_filter.c
	rm -f net
	@CFLAGS=${CFLAGS} SHLICC2=0 ./configure -q --with-gcc="${CC}"
	rm -f net

bpf_filter.c:	bpf/net/bpf_filter.c
	ln -fs bpf/net/bpf_filter.c .

depend:	Makefile
	@if [ $${M_DEPEND-"0"} -eq 0 ]; then \
		${MK} depend ; \
	fi

clean cleandir:
	@if [ -f Makefile ]; then \
		${MK} distclean ; \
	fi
	rm -f bpf_filter.c

install:
	install -c -m 444 -o ${BINOWN} -g ${BINGRP} libpcap.a /usr/lib
	install -c -m 444 -o ${BINOWN} -g ${BINGRP} pcap.h pcap-namedb.h \
		/usr/include

obj objdir mansourceinstall:

.include <${.CURDIR}/../../Makefile.inc>
.include <bsd.prog.mk>

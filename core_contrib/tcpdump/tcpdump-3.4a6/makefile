OPTIONAL_DEFINES= -DHAVE_TOKEN -DHAVE_C_HDLC -DINET6 -DIPSEC -DNRL_COMPAT
MK=${MAKE} -f Makefile CC="${LDCC}" CCOPT="${CFLAGS} ${OPTIONAL_DEFINES}" \
	 BINDEST=${BINDIR} MANDEST=${MANDIR}
MAN1=tcpdump.0

all:	Makefile manpages
	@${MK}

Makefile:
	@SHLICC2=0 ./configure -q --with-gcc="${GCC}"

depend:	Makefile
	@if [ $${M_DEPEND-"0"} -eq 0 ]; then \
		${MK} depend ; \
	fi

clean cleandir:
	@if [ -f Makefile ]; then \
		${MK} distclean ; \
	fi
	rm -f ${MAN1} Makefile

install: realinstall maninstall

realinstall:
	install -c -s -o ${BINOWN} -g ${BINGRP} -m 555 tcpdump ${BINDIR}

obj objdir:

.include <${.CURDIR}/../../Makefile.inc>
.include <bsd.prog.mk>

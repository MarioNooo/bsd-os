CC=	shlicc
BINDIR= /usr/sbin
MANDIR=	/usr/share/core_contrib/man
OWNER=	root
GROUP=	bin
MODE=	4555
SRC=traceroute.c ifaddrlist.c savestr.c version.c
OPTIONAL_DEFINES=-DNRL_COMPAT

all:	traceroute traceroute.0

traceroute: ${SRC} Makefile
	@make -f Makefile

Makefile:
	@CC="${CC}" ./configure
	sed -e '/DEFS/s;^DEFS *= *\(.*\)$$;DEFS = ${OPTIONAL_DEFINES} \1;' \
	< Makefile > Makefile.new && mv Makefile.new Makefile

traceroute.0:	Makefile traceroute.8
	groff -mtty-char -Tascii -man traceroute.8 > traceroute.0

clean cleandir:
	@if [ -f Makefile ]; then \
		make -f Makefile clean distclean ; \
	fi
	rm -f traceroute.0

depend:	Makefile
	make -f Makefile depend

version.c: Makefile
	make -f Makefile version.c

install:	traceroute traceroute.0
	install -c -s -o ${OWNER} -g ${GROUP} -m ${MODE} traceroute ${BINDIR}
	install -c -o bin -g bin -m 444 traceroute.0 ${MANDIR}/cat1

mansourceinstall:	traceroute.8
	install -c -o bin -g bin -m 444 traceroute.8 ${MANDIR}/man1
	
obj objdir:

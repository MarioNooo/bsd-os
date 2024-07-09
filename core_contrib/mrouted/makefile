CC=		shlicc
CFLAGS=		-O2
PROGS=		mrouted mrinfo mtrace map-mbone
MAN_SRCS=	mrouted.8 mrinfo.8 mtrace.8 map-mbone.8
MANS=		${MAN_SRCS:S/.8/.0/g}
MAKE_MAN=	groff -mtty-char -Tascii -man
BINDIR=		/usr/sbin
MANDIR=		/usr/share/core_contrib/man/cat8

.SUFFIXES+=	.0 .8

all:	${MANS}
	make -f Makefile CC="${CC}" CFLAGS="${CFLAGS}"

${MANS}:	${MAN_SRCS}
	${MAKE_MAN} ${.TARGET:S/.0/.8/g} > ${.TARGET}

install:	${MANS}
	for prog in ${PROGS}; do \
		install -c -s -o bin -g bin -m 555 $$prog ${BINDIR} ; \
	done
	for man in ${MANS}; do \
		install -c -o bin -g bin -m 444 $$man ${MANDIR} ; \
	done	

depend:
	make -f Makefile CC="${CC}" CFLAGS= MSTAT_SRCS= MSTAT_OBJS= depend

clean:
	make -f Makefile clean
	rm -f *.0 y.tab.h

cleandir:	clean
	rm -f .depend TAGS

obj objdir depend lint mansourceinstall:

tags:
	etags *.[ch]

true:

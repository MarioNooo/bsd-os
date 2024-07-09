#
# Makefile for mrouted, a multicast router, and its auxiliary programs,
# map-mbone and mrinfo.
#
# Makefile,v 1.1.1.1 1995/12/14 19:28:13 jch Exp
#
# If the multicast header files are not in the standard place on your system,
# define MCAST_INCLUDE to be an appropriate `-I' options for the C compiler.
#
#MCAST_INCLUDE=	-I/sys
#
# Uncomment the following eight lines if you want to use David Thaler's
# CMU SNMP daemon support.
#
#SNMPDEF=	-DSNMP
#SNMPLIBDIR=	-Lsnmpd -Lsnmplib
#SNMPLIBS=	-lsnmpd -lsnmp
#CMULIBS= snmpd/libsnmpd.a snmplib/libsnmp.a
#MSTAT=		mstat
#SNMPC=		snmp.c
#SNMPO=		snmp.o
#SNMPCLEAN=	snmpclean
# End SNMP support
#
# Uncomment the following three lines if you want to use RSRR (Routing
# Support for Resource Reservations), currently used by RSVP.
#RSRRDEF=	-DRSRR
#RSRRC=		rsrr.c
#RSRRO=		rsrr.o
#
LDFLAGS=
CFLAGS=		-O ${MCAST_INCLUDE} ${SNMPDEF} ${RSRRDEF}	## SunOS, OSF1, FreeBSD
#CFLAGS=		-O ${MCAST_INCLUDE} ${SNMPDEF} ${RSRRDEF} -D_BSD_SIGNALS	## IRIX
#CFLAGS=		-O ${MCAST_INCLUDE} ${SNMPDEF} ${RSRRDEF} -DSYSV -DSUNOS5	## Solaris 2.x
#LIB2=-lsocket -lnsl	## Solaris 2.x
LIBS=		${SNMPLIBDIR} ${SNMPLIBS} ${LIB2}
LINTFLAGS=	${MCAST_INCLUDE}
IGMP_SRCS=	igmp.c inet.c kern.c
IGMP_OBJS=	igmp.o inet.o kern.o
ROUTER_SRCS=	config.c cfparse.y main.c route.c vif.c prune.c callout.c \
		${SNMPC} ${RSRRC}
ROUTER_OBJS=	config.o cfparse.o main.o route.o vif.o prune.o callout.o \
		${SNMPO} ${RSRRO}
MAPPER_SRCS=	mapper.c
MAPPER_OBJS=	mapper.o
MRINFO_SRCS=	mrinfo.c
MRINFO_OBJS=	mrinfo.o
MTRACE_SRCS=	mtrace.c
MTRACE_OBJS=	mtrace.o
MSTAT_SRCS=	mstat.c 
MSTAT_OBJS=	mstat.o
HDRS=		defs.h dvmrp.h route.h vif.h prune.h
SRCS= ${IGMP_SRCS} ${ROUTER_SRCS} ${MAPPER_SRCS} ${MRINFO_SRCS} \
      ${MTRACE_SRCS} ${MSTAT_SRCS}
OBJS= ${IGMP_OBJS} ${ROUTER_OBJS} ${MAPPER_OBJS} ${MRINFO_OBJS} \
      ${MTRACE_OBJS} ${MSTAT_OBJS}
DISTFILES= ${SRCS} ${HDRS} LICENSE Makefile mrouted.conf \
	   mrouted map-mbone mrinfo mtrace mrouted.8
LIBC=	/lib/libc.a

all: mrouted map-mbone mrinfo mtrace ${MSTAT}

mrouted: ${IGMP_OBJS} ${ROUTER_OBJS} ${LIBC} ${CMULIBS}
	rm -f $@
	${CC} ${LDFLAGS} -o $@ ${CFLAGS} ${IGMP_OBJS} ${ROUTER_OBJS} ${LIBS}

mrouted.defs: multicast.defs dvmrp.defs smi.defs pim.defs
	cat smi.defs multicast.defs dvmrp.defs pim.defs > mrouted.defs

multicast.defs: multicast.my
	xmosy multicast.my > multicast.defs

dvmrp.defs: dvmrp.my
	xmosy dvmrp.my > dvmrp.defs

map-mbone: ${IGMP_OBJS} ${MAPPER_OBJS}
	rm -f $@
	${CC} ${LDFLAGS} -o $@ ${CFLAGS} ${IGMP_OBJS} ${MAPPER_OBJS} ${LIB2}

mrinfo: ${IGMP_OBJS} ${MRINFO_OBJS}
	rm -f $@
	${CC} ${LDFLAGS} -o $@ ${CFLAGS} ${IGMP_OBJS} ${MRINFO_OBJS} ${LIB2}

mtrace: ${IGMP_OBJS} ${MTRACE_OBJS}
	rm -f $@
	${CC} ${LDFLAGS} -o $@ ${CFLAGS} ${IGMP_OBJS} ${MTRACE_OBJS} ${LIB2}

mstat: ${MSTAT_OBJS} snmplib/libsnmp.a
	rm -f $@
	${CC} ${LDFLAGS} -o $@ ${CFLAGS} ${MSTAT_OBJS} -Lsnmplib -lsnmp

snmpd/libsnmpd.a:
	(cd snmpd; make)

snmplib/libsnmp.a:
	(cd snmplib; make)

clean: FRC ${SNMPCLEAN}
	rm -f ${OBJS} core mrouted map-mbone mrinfo mstat mtrace tags TAGS

snmpclean:	FRC
	-(cd snmpd; make clean)
	-(cd snmplib; make clean)

depend: FRC
	mkdep ${CFLAGS} ${SRCS}

lint: FRC
	lint ${LINTFLAGS} ${SRCS}

tags: ${IGMP_SRCS} ${ROUTER_SRCS}
	ctags ${IGMP_SRCS} ${ROUTER_SRCS}

cflow:	FRC
	cflow ${MCAST_INCLUDE} ${IGMP_SRCS} ${ROUTER_SRCS} > cflow.out

cflow2:	FRC
	cflow -ix ${MCAST_INCLUDE} ${IGMP_SRCS} ${ROUTER_SRCS} > cflow2.out

rcflow:	FRC
	cflow -r ${MCAST_INCLUDE} ${IGMP_SRCS} ${ROUTER_SRCS} > rcflow.out

rcflow2:	FRC
	cflow -r -ix ${MCAST_INCLUDE} ${IGMP_SRCS} ${ROUTER_SRCS} > rcflow2.out

TAGS: FRC
	etags ${SRCS}

dist: ${DISTFILES}
	sed -e '/^# DO NOT PUT ANYTHING/,$$d' \
	    -e '/^MCAST_INCLUDE=/s/=.*$$/=/' \
	    -e '/^LDFLAGS=/s/=.*$$/=/' \
		< Makefile > Makefile.dist
	mv Makefile Makefile.save
	cp Makefile.dist Makefile
	rm -f mrouted.tar.gz
	tar cvf - ${DISTFILES} | gzip -9 > mrouted.tar.gz
	mv Makefile.save Makefile

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.

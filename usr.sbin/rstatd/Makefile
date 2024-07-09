#	BSDI Makefile,v 2.7 2001/03/27 21:00:45 seebs Exp

RPCGEN=	rpcgen

PROG=	rstatd
MAN8=	rstatd.0
SRCS=	rstat_svc.c rstat_proc.c
CFLAGS+=-I/usr/include/rpcsvc
CLEANFILES+=rstat_svc.c

DPADD+=	${LIBRPCSVC} ${LIBUTIL} ${LIBKVM}
LDADD+=	-lrpcsvc -lutil -lkvm

.depend: rstat_svc.c

rstat_svc.c: /usr/include/rpcsvc/rstat.x
	cd /usr/include/rpcsvc ; \
		$(RPCGEN) -C -s udp -o ${.OBJDIR}/${.TARGET} rstat.x

.include <bsd.prog.mk>

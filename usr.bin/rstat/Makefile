#	BSDI Makefile,v 2.6 1996/09/19 14:24:36 bostic Exp

RPCGEN=	rpcgen

PROG=	rstat
SRCS=	rstat.c rstat_clnt.c
CFLAGS+=-I/usr/include/rpcsvc
CLEANFILES+=rstat_clnt.c

LINKS+=	${BINDIR}/rstat ${BINDIR}/rup
MLINKS+=rstat.1 rup.1

DPADD+=	${LIBRPCSVC}
LDADD+=	-lrpcsvc

.depend: rstat_clnt.c

rstat_clnt.c: /usr/include/rpcsvc/rstat.x
	cd /usr/include/rpcsvc && \
		$(RPCGEN) -l -o ${.OBJDIR}/${.TARGET} rstat.x

.include <bsd.prog.mk>

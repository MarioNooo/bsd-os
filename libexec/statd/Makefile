# BSDI Makefile,v 1.1 1998/03/19 20:20:20 don Exp

PROG =		statd
SRCS =		client.c sm_inter_svc.c statd.c sm_procs.c
MAN8 =		statd.0

RPCGEN =	rpcgen -L -C
RPCSRCDIR =	/usr/include/rpcsvc
RPCSRC = 	${RPCSRCDIR}/sm_inter.x

LOCKD =		${.CURDIR}/../lockd

.PATH: ${LOCKD} ${RPCSRCDIR}

DPADD =		${LIBRPCSVC}
LDADD =		-lrpcsvc

CFLAGS +=        -I${RPCSRCDIR}

CLEANFILES += 	sm_inter_svc.c sm_inter_cln.c notify.o notify

sm_inter_svc.c: ${RPCSRC}
	${RPCGEN} -m -o ${.TARGET} ${RPCSRC}

sm_inter_cln.c: ${RPCSRC}
	${RPCGEN} -l -o ${.TARGET} ${RPCSRC}

notify : notify.o sm_inter_cln.o ${DBADD}
	${CC} ${CFLAGS} -o ${.TARGET} ${.ALLSRC} ${LDADD}

.include <bsd.prog.mk>

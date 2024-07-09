# BSDI Makefile,v 1.2 1998/11/25 22:40:23 don Exp

PROG =		lockd
SRCS =		client.c kern.c nlm_prot_svc.c lockd.c nlm_procs.c svc_run.c
MAN8 =		lockd.0

SYSSRCDIR=	${.CURDIR}/../../sys

RPCGEN = 	rpcgen -L -C
RPCSRCDIR =	/usr/include/rpcsvc
RPCSRC = 	${RPCSRCDIR}/nlm_prot.x

DPADD =		${LIBRPCSVC}
LDADD =		-lrpcsvc 

.PATH: ${RPCSRCDIR}

CFLAGS+=	-I${RPCSRCDIR} -I${SYSSRCDIR}


CLEANFILES+= nlm_prot_svc.c

nlm_prot_svc.c: ${RPCSRC}
	${RPCGEN} -m -o ${.OBJDIR}/${.TARGET} ${RPCSRC}

.depend: nlm_prot_svc.c

.include <bsd.prog.mk>

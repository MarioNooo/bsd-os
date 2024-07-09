#	BSDI	Makefile,v 2.2 1998/09/22 13:10:35 torek Exp
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.if ${MACHINE} == "sparc_v9"
MCH=	sparc
.else
MCH=	${MACHINE}
.endif

MAN8=	adduser_info.0 intro.0 kerberos.0 rc.0 sticky.0

#SUBDIR=	man8.i386 man8.hp300 man8.tahoe man8.vax
SUBDIR=	man8.${MCH}

.include <bsd.prog.mk>

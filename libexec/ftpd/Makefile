#	@(#)Makefile	8.2 (Berkeley) 4/4/94
#	Makefile,v 1.9 2003/10/21 23:04:54 polk Exp

PROG=	ftpd
MAN8=	ftpd.0
MAN5=	ftpconfig.0
CDIR=	${DESTDIR}/etc/ftpd
SRCS=	config.c ftpd.c ftpcmd.c logwtmp.c popen.c auth.c private.c misc.c
SRCS+=	ipfwcircuit.c

# ALLOW_CORE_DROPS=1
REQUIRE_LS=1

LSDIR=	../../bin/ls
.if defined(REQUIRE_LS) || exists(${LSDIR}/ls.c)
.PATH:	${.CURDIR}/${LSDIR}
CFLAGS+=-DINTERNAL_LS -Dmain=ls_main -I${.CURDIR} -I${.CURDIR}/${LSDIR} -Dunix
SRCS+=	ls.c cmp.c print.c stat_flags.c util.c
.endif

CFLAGS+=-DSETPROCTITLE -Wall

# If ALLOW_CORE_DROPS is defined then ftpd will drop core in /tmp
# if it gets an unexpected signal

.if defined(ALLOW_CORE_DROPS)
LDCC=	cc
CFLAGS+=-DALLOW_CORE_DROPS
CFLAGS+=-g
LDFLAGS+=-g
.endif

LDADD=	-lutil
DPADD=	${LIBUTIL}

CLEANFILES+=ftpcmd.c y.tab.h

afterinstall:
	install -c -m 444 ${.CURDIR}/ftpconfig.sample ${CDIR}/config.sample
	[ ! -r ${CDIR}/config ] && \
		install -c -m 444 ${.CURDIR}/ftpconfig ${CDIR}/config || exit 0

.include <bsd.prog.mk>

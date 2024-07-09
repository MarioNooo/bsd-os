#
# Copyright (c) 1992, 1995 Berkeley Software Design, Inc. All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI Makefile,v 2.5 2003/06/05 23:45:03 polk Exp
#
# Compile time options:
#    -UDEBUG					general debug info
#    -UDEBUG_PARSER				to debug parser
#    -UDEBUG_WHEN				debug when.c
#    -UNEED_USLEEP				system doesn't have usleep()
#    -DPROGNAME=varname				where to find argv[0]
#    -UNOHDRS					no headers on list
#    -DCACHESIZE=41				cache size for user names
#    -DAT_DEFSHELL=\"/bin/sh\"			if no env["SHELL"]
#    -DAT_DIR=\"/var/spool/at/\"		at spool directory
#    -DAT_LOCKDIR=\"past\"			dir in AT_DIR for exec'd files
#    -DAT_LOCK=\"at.counter\"			job id counter
#    -DAT_RUNLOCK=\"atrun.lock\"		lockfile for atrun
#    -DAT_ALLOWED=\"/usr/libexec/at_allowed\"	access control
#    -DAT_TMP=\"/tmp\"				where cmds are exec'd from
#
# Debug when.c:
#     cc -DDEBUG_WHEN when.c datelex.o dateyacc.o errlib.o -ll -lutil -o when

.if defined(DEBUG)
LDCC=cc
CFLAGS=-g -DDEBUG=${DEBUG} -DYYDEBUG=1
LDADD+=-lefence
.endif

.if defined(GPROF)
CFLAGS+=-pg
.endif

CFLAGS+=-I. -I${.CURDIR}
LFLAGS+=-i

.if defined(DEBUG) || defined(GPROF)
NOMAN=	noman
YFLAGS+=-tv
.y.o:
	${YACC} ${YFLAGS} ${.IMPSRC}
	${CC} ${CFLAGS} -c y.tab.c -o ${.TARGET}
.l.o:
	${LEX} ${LFLAGS} ${.IMPSRC}
	${CC} ${CFLAGS} -c lex.yy.c -o ${.TARGET}
.endif

CFLAGS+=-DPROGNAME=progname
PROG=	at
SRCS=	at.c readqueue.c queuejob.c sched.c when.c \
	utils.c errlib.c dateyacc.y datelex.l
DPADD+=	${LIBL} ${LIBUTIL}
LDADD+=	-ll -lutil
BINOWN=	root
BINMODE=4555
LIBEXEC=/usr/libexec
MLINKS+=at.1 atq.1
MLINKS+=at.1 atrm.1
MLINKS+=at.1 at_allowed.8
LINKS+=${BINDIR}/at ${BINDIR}/atq
LINKS+=${BINDIR}/at ${BINDIR}/atrm
CLEANFILES+=y.tab.h

datelex.o: dateyacc.o

afterinstall:
	-[ -d ${DESTDIR}/var/spool/at ] || mkdir -p ${DESTDIR}/var/spool/at
	chown root ${DESTDIR}/var/spool/at
	chmod 755 ${DESTDIR}/var/spool/at
	:
	-[ -d ${DESTDIR}/var/spool/at/past ] || \
	    mkdir ${DESTDIR}/var/spool/at/past
	chown root ${DESTDIR}/var/spool/at/past
	chmod 755 ${DESTDIR}/var/spool/at/past
	:
	install -c -o root -g bin -m 644 \
			${.CURDIR}/at.counter ${DESTDIR}/var/spool/at
	install -c -o root -g bin -m 755 \
			${.CURDIR}/at_allowed ${DESTDIR}${LIBEXEC}

.include <bsd.prog.mk>

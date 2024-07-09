# Make `routed` for FreeBSD
# $FreeBSD: src/sbin/routed/Makefile,v 1.8.2.1 2001/04/25 10:58:50 ru Exp $
# $FreeBSD: src/sbin/routed/Makefile,v 1.8.2.1 2001/04/25 10:58:50 ru Exp $

PROG=   routed
BINDIR= /usr/sbin
SRCS=   if.c input.c main.c output.c parms.c radix.c rdisc.c table.c trace.c md5c.c
MAN8=   routed.0
SUBDIR= rtquery
#LDADD+= -lmd
#DPADD+= ${LIBMD}
#COPTS= -g -DDEBUG -W -Wall -Wcast-align -Wcast-qual -Winline -Wpointer-arith -Wnested-externs -Wwrite-strings -Wunused

afterinstall:
	ln -sf ../usr/sbin/routed ${DESTDIR}/sbin/routed

.include <bsd.prog.mk>

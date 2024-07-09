#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	syslogd
SRCS=	syslogd.c ttymsg.c md5c.c
.PATH:	${.CURDIR}/../../usr.bin/wall ${.CURDIR}/../../libexec/chap_md5
CFLAGS+=-I${.CURDIR}/../../libexec/chap_md5
MAN5=	syslog.conf.0
MAN8=	syslogd.0

.include <bsd.prog.mk>

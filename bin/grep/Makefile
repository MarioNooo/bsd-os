#	BSDI	Makefile,v 2.2 1995/08/10 17:30:19 bostic Exp

PROG=	grep
SRCS=	dfa.c grep.c kwset.c obstack.c regex.c search.c
CFLAGS+=-DGREP -DSTDC_HEADERS -DHAVE_STRING_H -DHAVE_SYS_PARAM_H
CFLAGS+=-DHAVE_UNISTD_H -DHAVE_GETPAGESIZE -DHAVE_MEMCHR
CFLAGS+=-DHAVE_STRERROR -DHAVE_VALLOC
MLINKS=	grep.1 egrep.1 grep.1 fgrep.1
LINKS=  ${BINDIR}/grep ${BINDIR}/egrep ${BINDIR}/grep ${BINDIR}/fgrep

.include <bsd.prog.mk>

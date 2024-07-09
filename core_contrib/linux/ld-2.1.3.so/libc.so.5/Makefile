#	BSDI Makefile,v 1.1 2000/02/12 17:16:06 donn Exp

PROG=		libc.so.5
SRCS=		libc.s
NOMAN=		noman
BINDIR=		/linux/lib
STRIP=

LDCC=		${LD} -shared -rpath /linux/usr/i486-linux-libc5/lib \
		-h libc.so.5 -f liblinux.so.1 -f libc.so.5.3.12

libc.s:
	touch libc.s

CLEANFILES+=	libc.s

depend tags:

.include <bsd.prog.mk>

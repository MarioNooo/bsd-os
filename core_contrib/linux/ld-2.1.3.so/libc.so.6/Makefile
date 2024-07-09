#	BSDI Makefile,v 1.3 2001/01/23 04:01:45 donn Exp

PROG=		libc.so.6
SRCS=		libc-dummy.c
NOMAN=		noman
BINDIR=		/linux/lib
STRIP=

LDCC=		${CC} -shared -nodefaultlibs \
		-Wl,-rpath,/lib,-rpath,/linux/lib \
		-Wl,-h,libc.so.6 -Wl,-f,liblinux.so.2,-f,libc-2.1.3.so \
		-Wl,--version-script=${.CURDIR}/Versions -L${.OBJDIR}

.include <bsd.prog.mk>

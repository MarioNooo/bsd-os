#	BSDI Makefile,v 1.5 2000/04/14 03:14:10 donn Exp

PROG=	afdb

NOMAN=	noman

all: libc.so.5.3.12.atof.db libc-2.1.3.so.atof.db

libc.so.5.3.12.atof.db: afdb
	./afdb /linux/usr/i486-linux-libc5/lib/libc.so.5.3.12

libc-2.1.3.so.atof.db: afdb
	./afdb /linux/lib/libc-2.1.3.so

install:

CLEANFILES+=	libc.so.5.3.12.atof.db libc.so.5.3.12.ftoa.db \
		libc-2.1.3.atof.db libc-2.1.3.ftoa.db

.include <bsd.prog.mk>

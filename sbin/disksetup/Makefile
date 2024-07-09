#	BSDI Makefile,v 2.16 2003/07/08 21:53:53 polk Exp

.if ${MACHINE} == "sparc_v9"
MCH=	sparc
.else
MCH=	${MACHINE}
.endif

PROG=	disksetup
SRCS=	help.c
SRCS+=	disksetup.cc
SRCS+=	curses.c screen.cc showhelp.cc disk.cc
SRCS+=	field.cc filesys.cc util.cc dkcksum.c print.cc express.cc
SRCS+=	bootblock.cc interactive.cc edit.cc geometry.cc choosedisk.cc

.if ${MCH} == "i386"
SRCS+=	partition.cc
.endif

.if ${MCH} == "powerpc"
SRCS+=	partition.cc
SRCS+=	powerpc.cc
.endif

.if ${MCH} == "sparc"
.if exists(${.CURDIR}/../../sys/sparc/sunos/sun_disklabel.h)
C++FLAGS+=	-DSUNDSKLABEL=\"${.CURDIR}/../../sys/sparc/sunos/sun_disklabel.h\"
.endif
.endif

MAN8=	disksetup.0
CLEANFILES+=	help.c help.h

LDCC= g++ -static

MKDEP=	mkdep -I.

C++FLAGS+=	-I.

LDADD+=	-lncurses -lutil
DPADD+=	${LIBNCURSES} ${LIBUTIL}

MSGS=	${.CURDIR}/messages

help.h help.c: Makefile ${MSGS}/* ${MSGS}.${MCH}/*
	@rm -f help.h
	for i in ${MSGS}/[a-z]* ${MSGS}.${MCH}/[a-z]*; do \
		f=$$(basename $$i) ;\
		echo "int $${f}_lines = $$(wc -l < $$i);" ;\
		echo "char *$$f[] = {" ;\
		echo "extern char *$$f[];" >> help.h ;\
		echo "extern int $${f}_lines;" >> help.h ;\
		sed -e 's/.*/"&",/' < $$i ;\
		echo "0 };" ;\
	done > help.c

.depend: help.h help.c

.include <bsd.prog.mk>

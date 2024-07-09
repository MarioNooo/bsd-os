#	BSDI Makefile,v 1.4 2002/01/29 18:28:55 donn Exp
#	$NetBSD: Makefile,v 1.12 2001/07/13 17:00:35 matt Exp $

__bsdi__= 1

PROG=	ldd
SRCS=	ldd.c \
	xmalloc.c debug.c map_object.c load.c search.c headers.c paths.c
MAN=	ldd.1

.PATH:	${.CURDIR}/..

.ifdef __bsdi__
MACHINE_ARCH= ${MACHINE}
.endif

.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "sparc64")
CPPFLAGS+= -DELFSIZE=64
.else
CPPFLAGS+= -DELFSIZE=32
.endif
.if (${MACHINE_ARCH} == "sparc") || (${MACHINE_ARCH} == "sparc64") || \
    (${MACHINE_ARCH} == "arm")
CPPFLAGS+= -DVARPSZ
.endif

LDELFSO=${.CURDIR}/../../../libexec/ld.elf_so
.ifdef __bsdi__
CPPFLAGS+= -I${.CURDIR} -I${LDELFSO}/arch/${MACHINE_ARCH}
CPPFLAGS+= -DDDBCTL_MAXID=0
.endif
CPPFLAGS+= -I${LDELFSO} -DLIBDIR=\"${LIBDIR}\"	# -DDEBUG
CPPFLAGS+= -D_RTLD_SOURCE
.ifdef __bsdi__
CFLAGS+= ${CPPFLAGS}
LDCC=	cc
.endif
.PATH: ${LDELFSO}

LDSTATIC?= -static

.include <bsd.prog.mk>

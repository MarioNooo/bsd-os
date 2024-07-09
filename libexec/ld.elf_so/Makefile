#	BSDI Makefile,v 1.8 2002/12/10 22:20:58 donn Exp
#	$NetBSD: Makefile,v 1.57 2002/09/27 21:37:50 thorpej Exp $

__bsdi__=1

.ifndef __bsdi__

.include <bsd.own.mk>			# for OBJECT_FMT definition
.include <bsd.shlib.mk>			# for SHLINKINSTALLDIR definition

ARCHSUBDIR= ${MACHINE_CPU}
BINDIR=${SHLINKINSTALLDIR}

.else

.SUFFIXES:
.SUFFIXES: .o .s .S .c

OBJECT_FMT= ELF
MKPIC= yes
MACHINE_ARCH= ${MACHINE}
ARCHSUBDIR= ${MACHINE_ARCH}
BINDIR= /shlib

.endif

M= ${.CURDIR}/arch/${ARCHSUBDIR}

.if ((${MACHINE_ARCH} == "alpha") ||					\
     (${MACHINE_ARCH} == "arm") || (${MACHINE_ARCH} == "armeb") ||	\
     (${MACHINE_ARCH} == "hppa") ||					\
     (${MACHINE_ARCH} == "i386") ||					\
     (${MACHINE_ARCH} == "m68k") ||					\
     (${MACHINE_ARCH} == "mipsel") || (${MACHINE_ARCH} == "mipseb") ||	\
     (${MACHINE_ARCH} == "powerpc") ||					\
     (${MACHINE_ARCH} == "sh3eb") || (${MACHINE_ARCH} == "sh3el") ||	\
     (${MACHINE_ARCH} == "sparc") ||					\
     (${MACHINE_ARCH} == "sparc64") ||					\
     (${MACHINE_ARCH} == "x86_64") ||					\
     (${MACHINE_ARCH} == "vax")) &&					\
    ${OBJECT_FMT} == "ELF" && ${MKPIC} != "no"

.ifndef __bsdi__
PROG=	ld.elf_so
.else
PROG=	ld-bsdi.so
MAN8=	ld-bsdi.so.0
MLINKS+= ld-bsdi.so.8 ld.so.8
STRIP=
.endif

# Adds SRCS, CPPFLAGS, LDFLAGS, etc.  Must go first so MD startup source
# is first.
.if exists($M/Makefile.inc)
.include "$M/Makefile.inc"
.endif

.ifndef __bsdi__
CLIBOBJ!=	cd ${NETBSDSRCDIR}/lib/libc && ${PRINTOBJDIR}
.endif

SRCS+=	rtld.c reloc.c symbol.c malloc.c xmalloc.c xprintf.c debug.c \
	map_object.c load.c search.c headers.c paths.c

CPPFLAGS+= -DLIBDIR=\"${LIBDIR}\" -D_PATH_RTLD=\"${BINDIR}/${PROG}\"
CPPFLAGS+= -I${.CURDIR}
.ifdef __bsdi__
CPPFLAGS+= -I${M}
.endif
CPPFLAGS+= -DRTLD_LOADER
CPPFLAGS+= -D_RTLD_SOURCE
#CPPFLAGS+= -DDEBUG
#CPPFLAGS+= -DRTLD_DEBUG
#CPPFLAGS+= -DRTLD_DEBUG_RELOC
.ifdef __bsdi__
CPPFLAGS+= -DDDBCTL_MAXID=0
RELEASE!= uname -r
CPPFLAGS+= -DRELEASE=\"${RELEASE}\"
CFLAGS+= ${CPPFLAGS}
# The MALLOC_OPTIONS hack overcomes a bug in ld 2.10.1.
ENV=	MALLOC_OPTIONS=Z
.endif
#DBG=	-g
DBG=	-O3 -fomit-frame-pointer

LDADD+=	-non_shared -L${DESTDIR}/${LIBDIR} -lc_pic
.ifdef __bsdi__
LDADD+=	--version-script=${.CURDIR}/Versions
LIBC_PIC=${LIBC:S/libc/libc_pic/}
DPADD+=	${LIBC_PIC}
.else
LDADD+=	-L${CLIBOBJ} -L${DESTDIR}${LIBDIR} -non_shared -lc_pic
DPADD+=	${CLIBOBJ}/libc_pic.a ${LIBC_PIC}
${DPADD}:
	@true
.endif

.ifndef __bsdi__
.if ${SHLIBDIR} != ${LIBDIR}
CPPFLAGS+= -DRTLD_DEFAULT_LIBRARY_PATH=\"${SHLIBDIR}:${LIBDIR}\"
.endif
.endif

.ifdef __bsdi__
.S.o:
	${CPP} ${CPPFLAGS} ${.IMPSRC} | ${AS} -o ${.TARGET}
.endif

STRIPFLAG=

.PATH: $M

${PROG}: ${OBJS} ${DPADD}
	${ENV} ${LD} ${LDFLAGS} -o ${PROG} ${OBJS} ${LDADD}

.ifndef __bsdi__
.if ${SHLINKINSTALLDIR} != "/usr/libexec"
SYMLINKS+=	${SHLINKINSTALLDIR}/${PROG} /usr/libexec/${PROG}
.endif
.endif

.include <bsd.prog.mk>

.ifdef __bsdi__
LIBDIR?=${LIBC:H}

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/link_elf.h \
		${DESTDIR}/usr/include/link.h
.endif

.else

MAN=	ld.elf_so.1

.include <bsd.man.mk>
.endif

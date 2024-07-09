#	BSDI Makefile,v 1.6 2003/09/15 19:37:34 jch Exp

GLIBCVER=	2.1.3
PROG=		ld-${GLIBCVER}.so
DIST=		${.CURDIR}/../D
GLIBCDIR=	${DIST}/glibc-${GLIBCVER}
ELFDIR=		${GLIBCDIR}/elf
I386ELFDIR=	${GLIBCDIR}/sysdeps/i386/elf
I386DIR=	${GLIBCDIR}/sysdeps/i386
GENDIR=		${GLIBCDIR}/sysdeps/generic
STUBDIR=	${GLIBCDIR}/sysdeps/stub
LINUXDIR=	${GLIBCDIR}/sysdeps/unix/sysv/linux
LINUXI386DIR=	${GLIBCDIR}/sysdeps/unix/sysv/linux/i386
W32DIR=		${GLIBCDIR}/sysdeps/wordsize-${WORDSIZE}
GNUDIR=		${GLIBCDIR}/sysdeps/gnu
INCDIR=		${GLIBCDIR}/include
INTLDIR=	${GLIBCDIR}/intl
LOCALEDIR=	${GLIBCDIR}/locale
POSIXDIR=	${GLIBCDIR}/posix
STDIODIR=	${GLIBCDIR}/stdio-common
STRINGDIR=	${GLIBCDIR}/string
GINCDIR=	${.CURDIR}/../../gcc/gcc/ginclude
LIBLINUX=	${.CURDIR}/../liblinux.so.2
LIBLINUXCOM=	${.CURDIR}/../liblinux.common

SUBDIR=		libc.so.6 libc.so.5

# XXX
WORDSIZE=	32

INCS=		-nostdinc -I. -I${.CURDIR} -I${I386ELFDIR} -I${I386DIR} \
		-I${W32DIR} -I${GINCDIR} -I${GLIBCDIR} -I${STDIODIR} \
		-I${LINUXI386DIR} -I${LINUXDIR} -I${GENDIR} -I${ELFDIR} \
		-I${INCDIR} -I${DIST} -I${STUBDIR} -I${LIBLINUXCOM} \
		-include ${.CURDIR}/config.h \
		-include ${INCDIR}/libc-symbols.h
DEFS=		-undef -D__ELF__ -Dunix -Dlinux '-Asystem(posix)' \
		-DLD_SO_CACHE='"/linux/etc/ld.so.cache"' \
		-DGNULOCALEDIR='"/usr/share/locale"'
CFLAGS+=	-fPIC -g ${INCS} ${DEFS}
TFLAGS=		-k -i ${LIBLINUX} -i ${LIBLINUXCOM} \
		-d ../afdb/libc-2.1.3.so

LDSRCS=		dl-cache.c dl-debug.c dl-deps.c dl-environ.c \
		dl-error.c dl-fini.c dl-init.c dl-load.c dl-lookup.c \
		dl-minimal.c dl-misc.c dl-object.c dl-origin.c dl-profile.c \
		dl-reloc.c dl-runtime.c dl-sysdep.c dl-version.c \
		rtld.c
LIBSRCS=        SYS_libc.c __longjmp.S _itoa.c _strerror.c \
		bzero.c closedir.c environ.c errlist.c errno-loc.c \
		ffs.c getcwd.c getenv.c itoa-digits.c loadmsgcat.c \
		memchr.S memcpy.c memmove.c mempcpy.c memset.c \
		opendir.c rawmemchr.S readdir.c sbrk.c setjmp.S \
		stpcpy.S stpncpy.S strchr.S strcmp.c strcpy.c \
		strcspn.S strerror.c strncmp.c strncpy.c strpbrk.c \
		strrchr.S strsep.c strspn.S strstr.c
EMULSRCS=	bsdi.s brk.x dirent.x errno.x fcntl.x mman.x stat.x \
		trampoline.x trap.x unistd.x xxx.c
SRCS=		soinit.c ${LDSRCS} ${LIBSRCS} ${EMULSRCS} sofini.c

# The MALLOC_OPTIONS hack overcomes a bug in ld.
LDCC=		MALLOC_OPTIONS=Z ${LD} -shared -rpath /lib -rpath /linux/lib \
		-h ld-linux.so.2 --version-script=${.CURDIR}/Versions

NOMAN=          noman

BINDIR=		/linux/lib
LINKS=		${BINDIR}/${PROG} ${BINDIR}/ld-linux.so.2
STRIP=

TRUSTEDDIRS=	/linux/lib
trusted-dirs.h: ${ELFDIR}/gen-trusted-dirs.awk
	echo ${TRUSTEDDIRS} | \
	    awk -f ${ELFDIR}/gen-trusted-dirs.awk > trusted-dirs.h
.depend:	trusted-dirs.h
CLEANFILES+=	trusted-dirs.h

.PATH:		. ${.CURDIR} ${ELFDIR} ${I386ELFDIR} ${I386DIR} \
		${LINUXI386DIR} ${LINUXDIR} ${STDIODIR} ${STRINGDIR} \
		${LOCALEDIR} ${INTLDIR} ${POSIXDIR} ${GNUDIR} \
		${GENDIR} ${LIBLINUX} ${LIBLINUXCOM}

.include <bsd.prog.mk>

TRANSFORM=	${.CURDIR}/../transform/obj/transform

.SUFFIXES:
.SUFFIXES: .o .s .S .x .c

.x.o:
	${TRANSFORM} ${TFLAGS} ${.IMPSRC}
	${CC} -I${.CURDIR}/../liblinux.common -fPIC -O2 -g -c -o ${.TARGET} ${.PREFIX}.c || \
		(rm -f ${.PREFIX}.c; false)
	rm -f ${.PREFIX}.c

.S.o:
	${CPP} -notraditional -D__ASSEMBLER__ -DASSEMBLER -DPIC \
		${INCS} ${DEFS} ${.IMPSRC} | \
		${AS} ${AFLAGS} -o ${.TARGET}

.s.o:
	${CPP} -DLOCORE -D__PIC__ ${.IMPSRC} | as -o ${.TARGET}

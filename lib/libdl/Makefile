#	BSDI Makefile,v 1.2 2002/01/18 21:06:33 polk Exp

LIB=		dl
SRCS=		dlfcn_elf.c

NOARCHIVE=	noarchive
NOSTATIC=	nostatic
REALNOPROFILE=	realnoprofile

# For compatible changes, increment minor
# For incompatible changes use increment major
LDDYNMAJOR=	.0
LDDYNMINOR=	.1

LDELFSO=	${.CURDIR}/../../libexec/ld.elf_so
CFLAGS+=	-I${LDELFSO}/arch/${MACHINE} -I${LDELFSO} -I${.CURDIR}
CFLAGS+=	-DPIC

LIBDIR=		/shlib

MAN3=		dlopen.0

.include <bsd.lib.mk>

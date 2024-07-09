#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	route
MAN8=	route.0
SRCS=	route.c
CFLAGS+=-I.
CLEANFILES+=keywords.h
BINOWN=	root
BINMODE=4555
SYSDIR=${.CURDIR}/../../sys

CFLAGS+=-I${SYSDIR}

.if exists(${SYSDIR}/netccitt/x25.h)
SRCS+= ccitt_addr.c
CFLAGS+= -DX25
.endif

.if exists(${SYSDIR}/netns/ns.h)
CFLAGS+= -DXNS
.endif

.if exists(${SYSDIR}/netiso/iso.h)
CFLAGS+= -DISO
.endif

all: route ${MAN8}

keywords.h: keywords
	sed -e '/^#/d' -e '/^$$/d' ${.CURDIR}/keywords > _keywords.tmp
	tr a-z A-Z < _keywords.tmp | paste _keywords.tmp - | \
	    awk '{ \
		if (NF > 1) \
			printf "#define\tK_%s\t%d\n\t{\"%s\", K_%s},\n", \
			    $$2, NR, $$1, $$2 }' \
	    > ${.TARGET}
	rm -f _keywords.tmp

.include <bsd.prog.mk>

route route.o .depend lint tags: keywords.h

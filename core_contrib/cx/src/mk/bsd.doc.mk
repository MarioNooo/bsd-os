#	BSDI bsd.doc.mk,v 2.2 1996/09/13 14:43:00 prb Exp
#
#	@(#)bsd.doc.mk	8.1 (Berkeley) 8/14/93

PRINTER?=	ps

EQN?=		eqn -C -T${PRINTER}
GRIND?=		vgrind -f
INDXBIB?=	indxbib
PIC?=		pic -C
REFER?=		refer
ROFF?=		groff -T${PRINTER} -m${PRINTER} ${MACROS} ${PAGES}
SOELIM?=	soelim
TBL?=		tbl -C

# These aren't currently available
BIB?=		bib
GREMLIN?=	grn

.PATH: ${.CURDIR}

.if !target(all)
.MAIN: all
all: paper.ps
.endif

.if !target(paper.ps)
paper.ps: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
.endif

.if !target(print)
print: paper.ps
	lpr -P${PRINTER} paper.ps
.endif

.if !target(manpages)
manpages:
.endif

.if !target(obj)
obj:
.endif

.if !target(objdir)
objdir:
.endif

.if !target(depend)
depend:
.endif

clean cleandir:
	rm -f paper.* [eE]rrs mklog ${CLEANFILES}

FILES?=	${SRCS}
install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    Makefile ${FILES} ${EXTRA} ${DESTDIR}${BINDIR}/${DIR}

spell: ${SRCS}
	spell ${SRCS} | sort | comm -23 - spell.ok > paper.spell

BINDIR?=	/usr/share/doc
BINGRP?=	bin
BINOWN?=	bin
BINMODE?=	444

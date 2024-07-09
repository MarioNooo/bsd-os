# From FreeBSD Id: //depot/aic7xxx/freebsd/dev/aic7xxx/aicasm/Makefile#2 $
#
# $FreeBSD: src/sys/dev/aic7xxx/aicasm/Makefile,v 1.16 2002/09/26 21:46:19 gibbs Exp $

#PROG=	aicasm
PROG=	aicasm.fbsd

CSRCS=	aicasm.c aicasm_symbol.c
YSRCS=	aicasm_gram.y aicasm_macro_gram.y
LSRCS=	aicasm_scan.l aicasm_macro_scan.l

GENHDRS=	aicasm_gram.h aicasm_macro_gram.h

#SRCS=	${GENHDRS} ${CSRCS} ${YSRCS} ${LSRCS}
SRCS=	${CSRCS} ${YSRCS:S/.y/.c/g} ${LSRCS:S/.l/.c/g}
CLEANFILES+= ${GENHDRS} ${YSRCS:R:C/(.*)/\1.output/g}
DPADD+=	${LIBL}
LDADD+=	-ll

# Correct path for kernel builds
# Don't rely on the kernel's .depend file
.ifdef MAKESRCPATH
.PATH: ${MAKESRCPATH}
DEPENDFILE=
.endif

CFLAGS+= -nostdinc -I/usr/include -I.
.ifdef MAKESRCPATH
CFLAGS+= -I${MAKESRCPATH}
.endif
NOMAN=	noman
#YFLAGS= -b ${.TARGET:R} ${.TARGET:M*macro*:S/$(.TARGET)/-p mm/} -d
YFLAGS= ${.TARGET:M*macro*:S/$(.TARGET)/-p mm/} -d
LFLAGS+= ${.TARGET:M*macro*:S/$(.TARGET)/-Pmm/}

.ifdef AICASM_DEBUG
CFLAGS+= -DDEBUG -g
YFLAGS+= -t -v
LFLAGS+= -d
.endif

aicasm_gram.h: aicasm_gram.c
aicasm_macro_gram.h: aicasm_macro_gram.c
.depend: ${GENHDRS}

.include <bsd.prog.mk>

# Need to override default yacc/lex rules to move headers and lex.mm.c.
# Should not need these, and the yacc rule single-threads the build.
.y.c:
	${YACC} ${YFLAGS} ${.IMPSRC}
	mv y.tab.c ${.TARGET} && mv y.tab.h ${.TARGET:R}.h

aicasm_macro_scan.c:
	${LEX} ${LFLAGS} ${.IMPSRC}
	mv lex.mm.c ${.TARGET}

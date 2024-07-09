# BSDI Makefile,v 1.9 1998/11/11 18:26:16 polk Exp

PROG=	perl4
SRCS=	array.c cmd.c cons.c consarg.c doarg.c doio.c dolist.c dump.c eval.c \
	form.c hash.c perl.c regcomp.c regexec.c stab.c str.c toke.c \
	usersub.c util.c perly.c
CFLAGS+= -I. -I${.CURDIR}
.if (${MACHINE} != "powerpc")
SRCS+=	unexec.c
CFLAGS+=-DUNEXEC
.endif
LDADD=	-lm
DPADD=	${LIBM}
NOMAN=	noman

CLEANFILES+= perly.c perly.h y.tab.h y.tab.c

perly.c: perly.y perly.fixer
	${YACC} -d ${.IMPSRC}
	sh ${.CURDIR}/perly.fixer y.tab.c perly.c
	mv y.tab.h perly.h
	echo 'extern YYSTYPE yylval;' >>perly.h

.include "../../Makefile.inc"

.include <bsd.prog.mk>

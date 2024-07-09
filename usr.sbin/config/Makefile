#	BSDI Makefile,v 2.6 2002/04/24 17:34:42 prb Exp
#
#	@(#)Makefile	8.3 (Berkeley) 5/29/94

PROG=	config
SRCS=	files.c gram.y mgram.y hash.c main.c mkheaders.c mkioconf.c \
	mkmakefile.c mkswap.c mkswgeneric.c pack.c sem.c template.c util.c \
	variable.c mscan.c cscan.c
CFLAGS+=-I${.CURDIR} -I.
CLEANFILES=gram.c mgram.c cscan.c mscan.c cc.tab.h mm.tab.h cc.output mm.output
MAN8=	config.0
YFLAGS+=-v
CFLAGS+=-DCCDEBUG -DMMDEBUG


.include <bsd.prog.mk>

.depend: gram.c mgram.c cscan.c mscan.c

gram.c:	gram.y
	yacc ${YFLAGS} ${.CURDIR}/gram.y
	sed -e s/yy/cc/g -e s/YY/CC/g < y.tab.c > gram.c
	sed -e s/yy/cc/g -e s/YY/CC/g < y.tab.h > cc.tab.h
	rm -f y.tab.c y.tab.h
	mv y.output cc.output

mgram.c:	mgram.y
	yacc ${YFLAGS} ${.CURDIR}/mgram.y
	sed -e s/yy/mm/g -e s/YY/MM/g < y.tab.c > mgram.c
	sed -e s/yy/mm/g -e s/YY/MM/g < y.tab.h > mm.tab.h
	rm -f y.tab.c y.tab.h
	mv y.output mm.output

cscan.c:	scan.l
	lex ${.CURDIR}/scan.l
	sed -e s/yy/cc/g -e s/YY/CC/g < lex.yy.c > cscan.c
	rm -f lex.yy.c

mscan.c:	scan.l
	lex ${.CURDIR}/scan.l
	sed -e s/yy/mm/g -e s/YY/MM/g < lex.yy.c > mscan.c
	rm -f lex.yy.c

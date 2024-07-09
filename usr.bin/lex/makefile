PROG=	lex
CFLAGS+=-I. -I${.CURDIR}
SRCS=	ccl.c dfa.c ecs.c gen.c main.c misc.c nfa.c skel.c sym.c \
	tblcmp.c yylex.c
OBJS+=	parse.o scan.o
CLEANFILES+=lex.0 lex.1 libl.a libmain.o libyywrap.o parse.c parse.h scan.c
MLINKS= lex.1 flex.1

DIRFILE=${DESTDIR}/usr/share/info/dir
INSTINFO= \
	install-info --dir-file=${DIRFILE} --quiet --section='Compilers'

all: lex libl.a lex.0

parse.h parse.c: ${.CURDIR}/parse.y
	yacc -d $?
	sed '/extern char.*malloc/d' < y.tab.c >parse.tmp
	mv parse.tmp parse.c
	mv y.tab.h parse.h
	rm -f y.tab.c
scan.c: ${.CURDIR}/scan.l
	lex -is8 $?
lex.1: ${.CURDIR}/flex.1
	cp $? $@

libl.a: libmain.o libyywrap.o
	ar cru $@ $?
	ranlib $@

beforeinstall:
	ranlib libl.a
	install -c -o bin -g bin -m 444 libl.a  ${DESTDIR}/usr/lib
	install -c -o bin -g bin -m 444 ${.CURDIR}/FlexLexer.h  ${DESTDIR}/usr/include/g++
	ranlib -t ${DESTDIR}/usr/lib/libl.a
	(cd ${DESTDIR}/usr/lib && rm -f libl_p.a && ln -s libl.a libl_p.a)
	(cd ${DESTDIR}/usr/lib && rm -f libln.a && ln -s libl.a libln.a)
	(cd ${DESTDIR}/usr/lib && rm -f libln_p.a && ln -s libl.a libln_p.a)
	-[ -f ${DIRFILE} ] || \
		${MINSTALL} ${.CURDIR}/../../core_contrib/dir ${DIRFILE}
	${INSTINFO} ${.CURDIR}/MISC/texinfo/flex.info
	${MINSTALL} ${.CURDIR}/MISC/texinfo/flex.info ${DESTDIR}/usr/share/info
	ln -sf lex ${DESTDIR}/${BINDIR}/flex
	
.include <bsd.prog.mk>

SRCS .depend: parse.h

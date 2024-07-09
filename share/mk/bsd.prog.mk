#	BSDI bsd.prog.mk,v 2.9 2002/10/28 22:40:57 prb Exp
#
#	@(#)bsd.prog.mk	8.2 (Berkeley) 4/2/94

.if !defined(NOINCLUDE) && exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.SUFFIXES: .out .o .c .cc .c++ .cxx .C .y .l .s .9 .8 .7 .6 .5 .4 .3 .2 .1 .0 .ps

.9.0 .8.0 .7.0 .6.0 .5.0 .4.0 .3.0 .2.0 .1.0:
	groff -mtty-char -Tascii -man ${.IMPSRC} > ${.TARGET}

.9.ps .8.ps .7.ps .6.ps .5.ps .4.ps .3.ps .2.ps .1.ps:
	groff -Tps -man ${.IMPSRC} > ${.TARGET}

CFLAGS+=${COPTS}

COPY?=	-c
STRIP?=	-s

BINGRP?=	bin
BINOWN?=	bin
BINMODE?=	555

.if ${OSNAME} == "BSD/OS"
LDCC?=		shlicc
.else
LDCC?=		cc
.endif
MKDEP?=		mkdep

LIBC?=		/usr/lib/libc.a
LIBCOMPAT?=	/usr/lib/libcompat.a
LIBCURSES?=	/usr/lib/libcurses.a
LIBDBM?=	/usr/lib/libdbm.a
LIBDES?=	/usr/lib/libdes.a
LIBG++?=	/usr/lib/libg++.a
LIBL?=		/usr/lib/libl.a
LIBKDB?=	/usr/lib/libkdb.a
LIBKRB?=	/usr/lib/libkrb.a
LIBKVM?=	/usr/lib/libkvm.a
LIBM?=		/usr/lib/libm.a
LIBMP?=		/usr/lib/libmp.a
LIBPC?=		/usr/lib/libpc.a
LIBPLOT?=	/usr/lib/libplot.a
LIBRESOLV?=	/usr/lib/libresolv.a
LIBRPC?=	/usr/lib/librpc.a
LIBRPCSVC?=	/usr/lib/librpcsvc.a
LIBTERM?=	/usr/lib/libterm.a
LIBUTIL?=	/usr/lib/libutil.a

OBJDIR?=/usr/obj
STRIPDIR?=/usr/src

.if defined(SHAREDSTRINGS)
CLEANFILES+=strings
.c.o:
	${CC} -E ${CFLAGS} ${.IMPSRC} | xstr -c -
	@${CC} ${CFLAGS} -c x.c -o ${.TARGET}
	@rm -f x.c
.endif

.if defined(PROG)

.if !defined(SRCS)
SRCS=	${PROG}.c
.endif

.if (defined(SRCS) && ${SRCS} != "")
OBJS+=  ${SRCS:R:S/$/.o/g}
.endif

${PROG}: ${OBJS} ${LIBC} ${DPADD}
	${LDCC} ${LDFLAGS} -o ${.TARGET} ${OBJS} ${LDADD}

.if	!defined(MAN1) && !defined(MAN2) && !defined(MAN3) && \
	!defined(MAN4) && !defined(MAN5) && !defined(MAN6) && \
	!defined(MAN7) && !defined(MAN8) && !defined(MAN9) && \
	!defined(NOMAN)
MAN1=	${PROG}.0
.endif
.endif
.if !defined(NOMAN)
MANALL=	${MAN1} ${MAN2} ${MAN3} ${MAN4} ${MAN5} ${MAN6} ${MAN7} ${MAN8} ${MAN9}
.else
MANALL=
.endif
manpages: ${MANALL}

_PROGSUBDIR: .USE
.if defined(SUBDIR) && !empty(SUBDIR)
	@for entry in ${SUBDIR}; do \
		(echo "===> $$entry"; \
		if test -d ${.CURDIR}/$${entry}.${MACHINE}; then \
			cd ${.CURDIR}/$${entry}.${MACHINE}; \
		else \
			cd ${.CURDIR}/$${entry}; \
		fi; \
		${MAKE} ${.TARGET:S/realinstall/install/:S/.depend/depend/}); \
	done
.endif

.if !target(all)
.MAIN: all
all: ${PROG} ${MANALL} _PROGSUBDIR
.endif

.if !target(clean)
clean: _PROGSUBDIR
	rm -f a.out [Ee]rrs mklog ${PROG}.core ${PROG} ${OBJS} ${CLEANFILES}
.endif

.if !target(cleandir)
cleandir: _PROGSUBDIR
	rm -f a.out [Ee]rrs mklog ${PROG}.core ${PROG} ${OBJS} ${CLEANFILES}
	rm -f .depend ${PROG}.ps ${MANALL}
.endif

# some of the rules involve .h sources, so remove them from mkdep line
.if !target(depend)
depend: .depend _PROGSUBDIR
.depend: ${SRCS}
.if defined(PROG)
	${MKDEP} ${_MKDEP} ${CFLAGS:M-nostdinc} ${CFLAGS:M-[IDU]*} \
	    ${.ALLSRC:M*.c} ${.ALLSRC:M*.cc} ${.ALLSRC:M*.c++} ${.ALLSRC:M*.cxx} ${.ALLSRC:M*.C}
.endif
.endif

.if !target(install)
.if !target(beforeinstall)
beforeinstall:
.endif
.if !target(afterinstall)
afterinstall:
.endif

realinstall: _PROGSUBDIR
.if defined(PROG)
	install ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${INSTALLFLAGS} ${PROG} ${DESTDIR}${BINDIR}
.endif
.if defined(HIDEGAME)
	(cd ${DESTDIR}/usr/games; rm -f ${PROG}; ln -s dm ${PROG}; \
	    chown games.bin ${PROG})
.endif
.if defined(LINKS) && !empty(LINKS)
	@set ${LINKS}; \
	while test $$# -ge 2; do \
		l=${DESTDIR}$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		echo $$t -\> $$l; \
		rm -f $$t; \
		ln $$l $$t; \
	done; true
.endif

install: afterinstall maninstall
afterinstall: realinstall
realinstall: beforeinstall
.endif

.if !target(lint)
lint: ${SRCS} _PROGSUBDIR
.if defined(PROG)
	@${LINT} ${LINTFLAGS} ${CFLAGS} ${.ALLSRC} | more 2>&1
.endif
.endif

.if !target(obj)
.if defined(NOOBJ)
obj: _PROGSUBDIR
.else
obj: _PROGSUBDIR
	@cd ${.CURDIR}; rm -rf obj; \
	here=${.CURDIR}; dest=${OBJDIR}`echo $$here | sed 's,${STRIPDIR},,'`; \
	echo "$$here -> $$dest"; ln -s $$dest obj; \
	if test -d ${OBJDIR} -a ! -d $$dest; then \
		mkdir -p $$dest; \
	else \
		true; \
	fi
.endif
.endif

.if !target(objdir)
.if defined(NOOBJ)
objdir: _PROGSUBDIR
.else
objdir: _PROGSUBDIR
	@cd ${.CURDIR}; \
	if test -h obj; then \
		here=${.CURDIR}; objlnk=`ls -l obj | sed 's/.* //'`; \
		echo "$$here -> $$objlnk"; \
		if test ! -d $$objlnk; then \
			mkdir -p $$objlnk; \
		else \
			true; \
		fi; \
	else \
		true; \
	fi
.endif
.endif

.if !target(tags)
tags: ${SRCS} _PROGSUBDIR
.if defined(PROG)
	-ctags -f /dev/stdout ${.ALLSRC} | \
	    sed "s;${.CURDIR}/;;" > ${.CURDIR}/tags
.endif
.endif

.if !defined(NOMAN)
.include <bsd.man.mk>
.else
maninstall:
mansourceinstall:
.endif

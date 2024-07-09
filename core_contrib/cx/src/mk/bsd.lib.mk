#	BSDI bsd.lib.mk,v 2.24 2002/04/16 15:49:49 erwan Exp
#
#	@(#)bsd.lib.mk	8.3 (Berkeley) 4/22/94

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

LIBDIR?=	/usr/lib
LIBGRP?=	bin
LIBOWN?=	bin
LIBMODE?=	444

COPY?=		-c
STRIP?=		-s

BINGRP?=	bin
BINOWN?=	bin
BINMODE?=	555

MKDEP?=		mkdep
SHLIB_MAP?=	${DESTDIR}/etc/shlib.map

LDDYNMAJOR?=.0
LDDYNMINOR?=.0

# BEGIN XXX
.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc_v9"
AS_PIC?=	${AS} -K PIC
.else
AS_PIC?=	${AS}
.endif
# END XXX

.MAIN: all

# prefer .s to a .c, add .po, remove stuff not used in the BSD libraries
.SUFFIXES:
.SUFFIXES: .out .o .po .do .s .c .cc .c++ .cxx .C .f .y .l .8 .7 .6 .5 .4 .3 .2 .1 .0 .m4 .ps

.8.0 .7.0 .6.0 .5.0 .4.0 .3.0 .2.0 .1.0:
	groff -mtty-char -Tascii -man ${.IMPSRC} > ${.TARGET}

.8.ps .7.ps .6.ps .5.ps .4.ps .3.ps .2.ps .1.ps:
	groff -Tps -man ${.IMPSRC} > ${.TARGET}
.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}
	@${LD} -x -r ${.TARGET} -o ${.TARGET}.out
	@mv ${.TARGET}.out ${.TARGET}

.c.po:
	${CC} -pg ${CFLAGS} -c ${.IMPSRC} -o ${.TARGET}
	@${LD} -X -r ${.TARGET} -o ${.TARGET}.out
	@mv ${.TARGET}.out ${.TARGET}

.c.do:
	${CC} -fPIC ${CFLAGS} -c ${.IMPSRC} -o ${.TARGET}

.cc.o .c++.o .cxx.o .C.o:
	${C++C} ${C++FLAGS} -c ${.IMPSRC}
	@${LD} -x -r ${.TARGET} -o ${.TARGET}.out
	@mv ${.TARGET}.out ${.TARGET}

.cc.po .c++.po .cxx.po .C.po:
	${C++C} -pg ${C++FLAGS} -c ${.IMPSRC} -o ${.TARGET}
	@${LD} -X -r ${.TARGET} -o ${.TARGET}.out
	@mv ${.TARGET}.out ${.TARGET}

.cc.do .c++.do .cxx.do .C.do:
	${C++C} -fPIC ${C++FLAGS} -c ${.IMPSRC} -o ${.TARGET}

.s.o:
	${CPP} -E ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
	    ${AS} -o ${.TARGET}
	@${LD} -x -r ${.TARGET} -o ${.TARGET}.out
	@mv ${.TARGET}.out ${.TARGET}

.s.po:
	${CPP} -E -DPROF ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
	    ${AS} -o ${.TARGET}
	@${LD} -X -r ${.TARGET} -o ${.TARGET}.out
	@mv ${.TARGET}.out ${.TARGET}

.s.do:
	${CPP} -E -D__PIC__ ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
	    ${AS_PIC} -o ${.TARGET}

MANALL=	${MAN1} ${MAN2} ${MAN3} ${MAN4} ${MAN5} ${MAN6} ${MAN7} ${MAN8}
manpages: ${MANALL}

.if !defined(NOSTATIC)
_NOSTATIC != \
	(echo 'MAP() { [ "X$$1" = "X-l${LIB}" ] && echo _static && exit 0; }'; \
	cat ${SHLIB_MAP}; \
	echo 'echo _nostatic'; \
	echo 'exit 0') | sh
.if ${_NOSTATIC} == "_nostatic"
NOSTATIC=nostatic
.endif
.endif

.if defined(NOPROFILE)
# XXX for compatibility; there must be a better way, though!
REALNOPROFILE=realnoprofile
NOSTATIC=nostatic  
NODYNAMIC=nodynamic
.endif

.if !defined(NOARCHIVE) || !defined(NOSTATIC)
_LIBS=lib${LIB}.a
.endif
.if !defined(REALNOPROFILE)
_LIBS+=lib${LIB}_p.a
.endif
.if !defined(NOSTATIC)
_LIBS+=lib${LIB}_s
.endif
.if !defined(NODYNAMIC)
LDDYNSUF=${LDDYNMAJOR}${LDDYNMINOR}
_LIBS+=lib${LIB}.so${LDDYNSUF}
.endif

all: ${_LIBS}
.if !defined(NOMAN)
all: ${MANALL}
.endif

.if (defined(SRCS) && ${SRCS} != "")
OBJS+=	${SRCS:R:S/$/.o/g}
.endif

lib${LIB}.a:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.a
	@if [ "X${SORTOBJS}" = "X" ]; then \
		${AR} cTq lib${LIB}.a $$(lorder ${OBJS} | tsort) ${LDADD} ; \
	else \
		${AR} cTq lib${LIB}.a ${SORTOBJS} ${LDADD} ; \
	fi
	${RANLIB} lib${LIB}.a

POBJS+=	${OBJS:.o=.po}
lib${LIB}_p.a:: ${POBJS}
	@echo building profiled ${LIB} library
	@rm -f lib${LIB}_p.a
	@if [ "X${SORTPOBJS}" = "X" ]; then \
		${AR} cTq lib${LIB}_p.a $$(lorder ${POBJS} | tsort) ${LDADD} ; \
	else \
		${AR} cTq lib${LIB}_p.a ${SORTOBJS} ${LDADD} ; \
	fi
	${RANLIB} lib${LIB}_p.a

lib${LIB}_s lib${LIB}_s.a:: lib${LIB}.a
	@echo building static shared ${LIB} library
	@eval $$( (echo 'MAP() { [ "X$$1" = "X-l${LIB}" ] || return;' \
	    'echo -n "/usr/sbin/shlib -m ${SHLIB_MAP} -t $$2 -d $$3"' \
		'"-n lib${LIB}_s -s lib${LIB}_s.a";' \
	    '[ -f ${.CURDIR}/loader.lib${LIB}.c ] &&' \
		'echo -n " -b ${.CURDIR}/loader.lib${LIB}.c";' \
	    '[ -f ${.CURDIR}/lib${LIB}.const ] &&' \
		'echo -n " -c ${.CURDIR}/lib${LIB}.const";' \
	    '[ -f ${.CURDIR}/lib${LIB}.except ] &&' \
		'echo -n " -e ${.CURDIR}/lib${LIB}.except";' \
	    '[ -f $$5 ] &&' \
		'echo -n " -i $$5";' \
	    'shift 5; echo -n " $$@";' \
	    'echo " lib${LIB}.a";' \
	    'exit 0; }'; cat ${SHLIB_MAP}) | sh)

DOBJS+=	${OBJS:.o=.do}
lib${LIB}.so${LDDYNSUF}:: ${DOBJS}
	@echo building dynamic shared ${LIB} library
	@if [ -f ${.CURDIR}/lib${LIB}.except ]; then \
		cat ${.CURDIR}/lib${LIB}.except | \
		  sed -e '/^_/d' -e 's/$$/\;/' > tmpexcept ; \
	fi
	@if [ -s tmpexcept ]; then \
		echo ${LDDYNSUF} | sed -e 's/./VERS_/' -e 's/$$/ {/' \
			> soexcept ; \
		echo 'local:' >> soexcept ; \
		cat tmpexcept >> soexcept ; \
		echo '};' >> soexcept ; \
		${CC} -shared \
		  -Wl,-h,lib${LIB}.so${LDDYNMAJOR},--version-script,soexcept \
		  -o lib${LIB}.so${LDDYNSUF} ${DOBJS} ${LDDYNDEP} ; \
	else \
		${CC} -shared -Wl,-h,lib${LIB}.so${LDDYNMAJOR} \
		  -o lib${LIB}.so${LDDYNSUF} ${DOBJS} ${LDDYNDEP} ; \
	fi

.if !target(clean)
clean:
	rm -f ${OBJS}
	rm -f ${POBJS}
	rm -f ${DOBJS}
	rm -f a.out *.o.out [Ee]rrs mklog ${CLEANFILES} tmpexcept soexcept lib${LIB}.a \
	    lib${LIB}_p.a lib${LIB}_s lib${LIB}_s.a lib${LIB}.so${LDDYNSUF}
.endif

.if !target(cleandir)
cleandir:
	rm -f ${OBJS}
	rm -f ${POBJS}
	rm -f ${DOBJS}
	rm -f a.out *.o.out [Ee]rrs mklog ${CLEANFILES} lib${LIB}.a \
	    lib${LIB}_p.a lib${LIB}_s lib${LIB}_s.a lib${LIB}.so${LDDYNSUF}
	rm -f ${MANALL} .depend
.endif

.if !target(depend)
depend: .depend
.depend: ${SRCS}
	${MKDEP} ${CFLAGS:M-[ID]*} ${AINC} ${.ALLSRC}
	@(TMP=/tmp/_depend$$$$; \
	    sed -e 's/^\([^\.]*\).o *:/\1.o \1.po \1.do:/' < .depend > $$TMP; \
	    mv $$TMP .depend)
.endif

.if !target(install)
.if !target(beforeinstall)
beforeinstall:
.endif

realinstall: beforeinstall
.if !defined(NOARCHIVE)
	${RANLIB} lib${LIB}.a
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} lib${LIB}.a \
	    ${DESTDIR}${LIBDIR}
	${RANLIB} -t ${DESTDIR}${LIBDIR}/lib${LIB}.a
.endif
.if !defined(REALNOPROFILE)
	${RANLIB} lib${LIB}_p.a
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    lib${LIB}_p.a ${DESTDIR}${LIBDIR}
	${RANLIB} -t ${DESTDIR}${LIBDIR}/lib${LIB}_p.a
.endif
.if !defined(NOSTATIC)
	@echo installing static shared ${LIB} library
	@eval $$( (echo 'MAP() { [ "X$$1" = "X-l${LIB}" ] || return;' \
	    'echo "SHARED=$$5;";' \
	    'echo "STUB=$$(dirname $$4)/$$(basename $$5).a";' \
	    'exit 0; }'; cat ${SHLIB_MAP}) | sh); \
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} lib${LIB}_s \
	    ${DESTDIR}$$SHARED; \
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} lib${LIB}_s.a \
	    ${DESTDIR}$$STUB; \
	${RANLIB} -t ${DESTDIR}$$STUB
	-@eval $$( (echo 'MAP() { [ "X$$1" = "X-l${LIB}" ] || return;' \
	    'echo "LDIR=$$(dirname $$4)"; exit 0; }'; \
	    cat ${SHLIB_MAP}) | sh); \
	[ -f ${.CURDIR}/loader.lib${LIB}.c ] && \
	    install -c -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
		${.CURDIR}/loader.lib${LIB}.c \
		${DESTDIR}$$LDIR/loader.lib${LIB}.c; \
	[ -f ${.CURDIR}/lib${LIB}.const ] && \
	    install -c -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
		${.CURDIR}/lib${LIB}.const ${DESTDIR}$$LDIR/lib${LIB}.const; \
	[ -f ${.CURDIR}/lib${LIB}.except ] && \
	    install -c -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
		${.CURDIR}/lib${LIB}.except ${DESTDIR}$$LDIR/lib${LIB}.except; \
	true
.endif
.if !defined(NODYNAMIC)
	@echo installing dynamic shared ${LIB} library
	@eval $$( (echo 'MAP() { [ "X$$1" = "X-l${LIB}" ] || return;' \
	    'echo "SHAREDDIR=$$(dirname $$5)"; exit 0; }'; \
	    cat ${SHLIB_MAP}; echo "echo SHAREDDIR=${LIBDIR}") | sh); \
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} lib${LIB}.so${LDDYNSUF} \
	    ${DESTDIR}$$SHAREDDIR/lib${LIB}.so${LDDYNSUF}
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

install: afterinstall
afterinstall: realinstall
.if !defined(NOMAN)
afterinstall: maninstall
.endif
.endif

.if !target(lint)
lint:
.endif

.if !target(tags)
tags: ${SRCS}
	-ctags -f /dev/stdout ${.ALLSRC:M*.c} | \
	    sed "s;\${.CURDIR}/;;" > ${.CURDIR}/tags
.endif

.include <bsd.man.mk>

OBJDIR?=/usr/obj
STRIPDIR?=/usr/src

.if !target(obj)
.if defined(NOOBJ)
obj:
.else
obj:
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
objdir:
.else
objdir:
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

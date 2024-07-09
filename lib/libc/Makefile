# BSDI Makefile,v 2.25 2003/06/18 16:41:21 polk Exp
LIB=c
AINC=	-I${.CURDIR}/${MACHINE}
CLEANFILES+=tags

# For compatible changes, increment minor
# For incompatible changes use increment major
LDDYNMAJOR=.3
LDDYNMINOR=.0

CFLAGS+=-DBSDI_LIBC

.if defined(WV_INST)
CFLAGS+=-DWV_INST
.endif

.include "${.CURDIR}/compat-43/Makefile.inc"
.include "${.CURDIR}/db/Makefile.inc"
.include "${.CURDIR}/gen/Makefile.inc"
.include "${.CURDIR}/gmon/Makefile.inc"
.include "${.CURDIR}/locale/Makefile.inc"
.include "${.CURDIR}/net/Makefile.inc"
# Put bind after net so we find the net versions of man pages if they exist
.include "${.CURDIR}/bind/Makefile.inc"
.include "${.CURDIR}/quad/Makefile.inc"
.include "${.CURDIR}/regex/Makefile.inc"
.include "${.CURDIR}/rpc/Makefile.inc"
.include "${.CURDIR}/stdio/Makefile.inc"
.include "${.CURDIR}/stdlib/Makefile.inc"
.include "${.CURDIR}/string/Makefile.inc"
.include "${.CURDIR}/sys/Makefile.inc"
.include "${.CURDIR}/threads/Makefile.inc"
.include "${.CURDIR}/inet6/Makefile.inc"

libc_pic.a: ${DOBJS}
	@echo building ${LIB} archive of PIC objects
	@${AR} cq libc_pic.a ${DOBJS}
	${RANLIB} libc_pic.a

WOBJS+= ${OBJS:.o=.wo}

KQSRCS=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c iordi3.c \
	lshldi3.c lshrdi3.c moddi3.c muldi3.c negdi2.c notdi2.c qdivrem.c \
	subdi3.c ucmpdi2.c udivdi3.c umoddi3.c xordi3.c
KSRCS=	bcmp.c ffs.c index.c mcount.c rindex.c strcat.c strcmp.c strcpy.c \
	strlen.c strncpy.c

libkern: libkern.gen libkern.${MACHINE}

libkern.gen: ${KQSRCS} ${KSRCS}
	cp -p ${.CURDIR}/quad/quad.h ${.ALLSRC} /sys/libkern

libkern.${MACHINE}:: ${KMSRCS}
.if defined(KMSRCS) && !empty(KMSRCS)
	cp -p ${.ALLSRC} /sys/libkern/${MACHINE}
.endif

beforeinstall: rpcbeforeinstall bindbeforeinstall tags
	install -c -o bin -g bin -m 444 tags ${DESTDIR}/var/db/libc.tags
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} libc_pic.a \
	    ${DESTDIR}${LIBDIR}
	${RANLIB} -t ${DESTDIR}${LIBDIR}/libc_pic.a

afterinstall:
.if !defined(NODYNAMIC)
	@echo installing WindView Instrumented dynamic shared ${LIB}-windview library
	@eval $$( (echo 'MAP() { [ "X$$1" = "X-l${LIB}-windview" ] || return;' \
	    'echo "SHAREDDIR=$$(dirname $$5)"; exit 0; }'; \
	    cat ${SHLIB_MAP}; echo "echo SHAREDDIR=${LIBDIR}") | sh); \
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    lib${LIB}-windview.so${LDDYNSUF} \
	    ${DESTDIR}$$SHAREDDIR/lib${LIB}-windview.so${LDDYNSUF}
.endif

tags: ${SRCS}
	ctags ${.ALLSRC:M*.c}
	egrep -o "^ENTRY(.*)|^FUNC(.*)|^SYSCALL(.*)" ${.ALLSRC:M*.s} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> tags; sort -o tags tags

depend: .depend
.depend: ${SRCS}
	${MKDEP} ${CFLAGS:M-[ID]*} ${AINC} ${.ALLSRC}
	@(TMP=/tmp/_depend$$$$; \
	    sed -e 's/^\([^\.]*\).o *:/\1.o \1.po \1.do \1.wo:/' < .depend > $$TMP; \
	    mv $$TMP .depend)

.include <bsd.lib.mk>

.SUFFIXES: .wo

.c.wo:
	${CC} -fPIC ${CFLAGS} -DWV_INST -c ${.IMPSRC} -o ${.TARGET}

.s.wo:
	${CPP} -E -D__PIC__ ${CFLAGS:M-[ID]*} -DWV_INST ${AINC} ${.IMPSRC} | \
	    ${AS_PIC} -o ${.TARGET}

lib${LIB}-windview.so${LDDYNSUF}:: ${WOBJS}
	@echo building WindView Instrumented dynamic shared ${LIB}-windview library
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
		  -Wl,-h,lib${LIB}-windview.so${LDDYNMAJOR},--version-script,soexcept \
		  -o lib${LIB}-windview.so${LDDYNSUF} ${WOBJS} ${LDDYNDEP} ; \
	else \
		${CC} -shared -Wl,-h,lib${LIB}-windview.so${LDDYNMAJOR} \
		  -o lib${LIB}-windview.so${LDDYNSUF} ${WOBJS} ${LDDYNDEP} ; \
	fi

all: libc_pic.a lib${LIB}-windview.so${LDDYNSUF}

#	BSDI makefile,v 1.5 2003/10/17 21:59:03 polk Exp

BINDIR=		/usr/bin
INSTALL?=	install

# probably should use "obj", but plain make likes to chdir into
# obj subdirectories, so for now...
COMPILEDIR=	compile

GMAKE=		gmake

all: compile/Makefile
	(cd compile; ${GMAKE})

clean:
	if [ -f compile/Makefile ]; then \
	    (cd compile; ${GMAKE} clean); \
	fi

cleandir:
	rm -rf compile

# use the -v line to see actual failures rather than summaries
selftest: compile/Makefile
#	(cd compile; ${GMAKE} -k check)
	(cd compile; ${GMAKE} -k RUNTESTFLAGS=-v check)

install: maninstall
	(cd compile; ${GMAKE} install)
	mv ${DESTDIR}/usr/lib/libgcc_s.so* ${DESTDIR}/shlib
	ln -f ${DESTDIR}${BINDIR}/gcc ${DESTDIR}${BINDIR}/cc
	ln -f ${DESTDIR}${BINDIR}/g++ ${DESTDIR}${BINDIR}/c++
	install -c -o bin -g bin -m 0444 compile/gcc/include/xmmintrin.h \
		compile/gcc/include/mmintrin.h ${DESTDIR}/usr/include/

MAN1= \
	gcc/doc/cpp.1			\
	gcc/f/g77.1			\
	gcc/doc/gcc.1			\
	gcc/java/gcj.1			\
	gcc/java/gcjh.1			\
	gcc/doc/gcov.1			\
	gcc/java/gij.1			\
	gcc/java/jcf-dump.1		\
	gcc/java/jv-convert.1		\
	gcc/java/jv-scan.1		\
	gcc/java/rmic.1			\
	gcc/java/rmiregistry.1

maninstall:
	for i in ${MAN1}; do \
	    o=`echo $$i | sed -e 's/.1$$/.0/' -e s#.*/##`; \
	    nroff -man $$i > ${DESTDIR}${MANDIR}1${MANSUBDIR}/$$o; \
	done; \

depend obj objdir:
	@-[ -d ${COMPILEDIR} ] || mkdir ${COMPILEDIR}
	@-[ -f ${COMPILEDIR}/Makefile ] || \
	    (cd ${COMPILEDIR}; sh ${.CURDIR}/configure \
		--prefix=/usr \
		--mandir=/usr/share/core_contrib/man \
		--infodir=/usr/share/info \
		--libdir=/usr/lib \
	    )

tags:

.include <bsd.prog.mk>

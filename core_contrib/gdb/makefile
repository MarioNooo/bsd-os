#	makefile,v 1.1 2003/06/21 06:33:00 donn Exp

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

install: maninstall
	(cd compile; ${GMAKE} install)

MAN1= \
	gdb/gdb.1

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

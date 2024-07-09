PREFIX=		/usr/
MAN1DIR=	${PREFIX}/share/core_contrib/man/cat1
MAN1SRC=	${PREFIX}/share/core_contrib/man/man1
DIRFILE=	${DESTDIR}/usr/share/info/dir

all:	Makefile glob/Makefile
	make -f Makefile CC=shlicc

clean:
	-test -f Makefile && make -f Makefile clean

cleandir: clean
	rm -f Makefile build.sh config.cache config.h config.log \
	config.status loadavg.c stamp-config glob/Makefile \
	glob/config.log glob/config.status


Makefile glob/Makefile:
	CC=shlicc ./configure \
		--prefix=${PREFIX} \
		--program-prefix=g \
		--mandir=${PREFIX}/share/core_contrib/man/ \
		--infodir=${PREFIX}/share/info/

install:
	-[ -f ${DIRFILE} ] || \
		install -c -o bin -g bin -m 444 ../dir ${DIRFILE}
	make -f Makefile install
	groff -mtty-char -Tascii -man make.1 > ${MAN1DIR}/gmake.0

depend: Makefile

mansourceinstall obj objdir tags:

.DEFAULT:
	make -f Makefile $@

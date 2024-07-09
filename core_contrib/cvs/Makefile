#	BSDI	Makefile,v 1.1 2003/08/08 03:49:44 polk Exp

.if ${MACHINE} == "powerpc"
CC="shlicc -g -DVA_LIST_IS_ARRAY"
.else
CC="shlicc -g"
.endif

all: D/config.status
	(cd D; make CC=${CC})

DIRFILE=/usr/share/info/dir

install: 
	(cd D; make install)
	strip /usr/bin/cvs
	nroff -mandoc D/man/cvs.1 > /usr/share/core_contrib/man/cat1/cvs.0
	nroff -mandoc D/man/cvs.5 > /usr/share/core_contrib/man/cat5/cvs.0
	nroff -mandoc D/man/cvsbug.8 > /usr/share/core_contrib/man/cat8/cvsbug.0
	-[ -f ${DIRFILE} ] || \
		install -c -o bin -g bin -m 644 ${.CURDIR}/../dir ${DIRFILE}
	install-info --dir-file=${DIRFILE} --quiet --section='Utilities' \
		--item='* Cvs: (cvs).			CVS revision control system.' \
		D/doc/cvs.info
	install-info --dir-file=${DIRFILE} --quiet --section='Utilities' \
		--item='* Cvs-Protocol: (cvsclient).	CVS client/server protocol.' \
		D/doc/cvsclient.info

clean cleandir:
	if [ -f D/config.status ]; then \
		(cd D; make distclean); \
	fi

depend obj objdir tags mansourceinstall:

D/config.status:
	(cd D; ./configure \
		--prefix=/usr \
		-libdir=/usr/share \
		--infodir=/usr/share/info \
		--with-krb4=/usr \
		--mandir=/usr/share/core_contrib/man \
	)

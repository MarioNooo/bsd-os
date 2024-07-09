#	BSDI	makefile,v 1.11 2002/06/25 18:15:00 polk Exp

SUBDIR=		bsdi cgi-src

MAKE_MAN=	groff -mtty-char -Tascii -man
MAN8_FILES=	ab apachectl httpd logresolve rotatelogs apxs
MAN1_FILES=	htpasswd htdigest dbmmanage


all:	Makefile build _PROGSUBDIR

build:
	${MAKE} -f Makefile

Makefile: Makefile.tmpl Makefile.inc
	CFLAGS="-DHARD_SERVER_LIMIT=4096 -DHAVE_SYSVSEM_SERIALIZED_ACCEPT -DUSE_FLOCK_SERIALIZED_ACCEPT" LIBS=-lm \
	./configure \
		--with-layout=BSDI \
		--with-perl=/usr/bin/perl \
		--without-confadjust \
		--enable-module=most \
		--enable-module=so \
		--enable-rule=EAPI \
		--add-module=src/modules/extra/mod_auth_syspw.c \
		--enable-shared=auth_syspw \
		--enable-shared=max \
		--server-uid=www \
		--server-gid=www \
		--htdocsdir=/usr/share/doc/apache

depend:	Makefile

obj:

clean:
	@if [ -f Makefile ]; then \
		make -f Makefile MAKE='make -f Makefile' clean; \
	fi
	@for dir in ${SUBDIR}; do \
		( cd ${SUBDIR}; make clean ); \
	done
	rm -f ${CLEANFILES}

cleandir:
	@if [ -f Makefile ]; then \
		make -f Makefile MAKE='make -f Makefile' distclean; \
	fi
	@for dir in ${SUBDIR}; do \
		( cd ${SUBDIR}; make cleandir ); \
	done
	rm -f ${CLEANFILES} Makefile config.status

beforeinstall:	all
	${MAKE} -f Makefile install
	-mkdir ${PREFIX}/docs
	ln -fs /usr/share/doc/apache ${PREFIX}/docs/apache
	-mkdir ${MANDIR}/cat1
	-mkdir ${MANDIR}/cat8
	for file in ${MAN8_FILES}; do \
		${MAKE_MAN} src/support/$$file.8 \
		    > ${MANDIR}/cat8/$$file.0 && \
		chown bin.bin ${MANDIR}/cat8/$$file.0 && \
		chmod 444 ${MANDIR}/cat8/$$file.0; \
	done
	for file in ${MAN1_FILES}; do \
		${MAKE_MAN} src/support/$$file.1 \
		    > ${MANDIR}/cat1/$$file.0 && \
		chown bin.bin ${MANDIR}/cat1/$$file.0 && \
		chmod 444 ${MANDIR}/cat1/$$file.0; \
	done

.include <bsd.prog.mk>
.include "Makefile.inc"

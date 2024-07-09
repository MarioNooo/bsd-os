#	BSDI	Makefile,v 1.5 2003/06/03 21:35:31 polk Exp

all: dist/config.status
	(cd dist; gmake)
	(cd dist/intl; gmake libintl.la)

install: 
	(cd dist; gmake install)
	(cd dist/intl/.libs; pax -rwv . /usr/lib)
	ldconfig

clean cleandir:
	if [ -f dist/config.status ]; then \
		(cd dist; gmake distclean); \
	fi

depend obj objdir tags maninstall mansourceinstall:

.include "../Makefile.inc"

dist/config.status:
	(cd dist; bash configure ${STARG} \
		--prefix=/usr \
		--infodir=/usr/share/info \
		--with-included-gettext \
	)

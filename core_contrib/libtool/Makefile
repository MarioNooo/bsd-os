#	BSDI	$Id: Makefile,v 1.3 2001/02/27 16:31:43 polk Exp $

all: dist/config.status
	cd dist; make

install: 
	cd dist; make install

clean cleandir:
	if [ -f dist/config.status ]; then \
		cd dist; gmake distclean; \
	fi

depend obj objdir tags mansourceinstall:

dist/config.status:
	cd dist; bash configure --prefix=/usr \
				--mandir=/usr/share/man \
				--infodir=/usr/share/info
	

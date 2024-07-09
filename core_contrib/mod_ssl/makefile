#	BSDI	makefile,v 1.2 2002/01/09 16:39:30 polk Exp

all:	Makefile
	make -f Makefile

install:
	make -f Makefile install

clean:
	if [ -e Makefile ]; then \
		make -f Makefile clean ; \
	fi

cleandir: clean
	rm -f Makefile config.status

obj objdir depend mansourceinstall:

Makefile:
	./configure \
		--with-apxs=/usr/sbin/apxs \
		--with-ssl=/usr

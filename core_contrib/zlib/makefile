#	BSDI	makefile,v 1.3 2002/03/19 00:00:44 polk Exp

all:
	@echo "Building is done as part of the install"

clean cleandir:
	make -f Makefile clean

install:
	./configure --shared 
	make -f Makefile clean
	make -f Makefile
	make -f Makefile prefix=/usr install
	./configure
	make -f Makefile clean
	make -f Makefile
	make -f Makefile prefix=/usr install

depend mansourceinstall obj objdir tags:

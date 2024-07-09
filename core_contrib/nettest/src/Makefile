
MAKE=make
FILES=README Makefile nettest.h nettest.c nettestd.c nettest.8

all:
	@if [ -f /bin/hostname -o -f /usr/bin/hostname ]; then \
		$(MAKE) `hostname`; \
	elif [ -f /bin/uname ]; then \
		$(MAKE) `uname -n`; \
	else \
		echo You must specify what machine you are on,;\
		echo e.g. \"make sun\", \"make cray2\", \"make pyramid\", \"make xmp\";\
		echo; \
	fi

#Cray UNICOS definitions
unicos70 unicos71 unicos80:
	$(MAKE) objs \
		INCLUDE="-I." \
		DEFINES="-DNAMEDPIPES -DHAS_PARSETOS" \
		LIBS="" OPT=""

unicos60 unicos61:
	$(MAKE) objs \
		INCLUDE="-I." \
		DEFINES="-DNAMEDPIPES -DNO_ISO" \
		LIBS="" OPT=""

#BSD definitions
4.3bsd 4.3taho:
	$(MAKE) objs \
		INCLUDE="-I." \
		DEFINES="-DWAIT3CODE -DNO_ISO" \
		LIBS="" OPT="-O"

4.4alpha:
	$(MAKE) objs \
		INCLUDE="" \
		DEFINES="-DWAIT3CODE -DBSD44 -DNAMEDPIPES -DNO_ISO" \
		OPT="-O" LIBS="-lcompat"

4.3reno:
	$(MAKE) objs \
		INCLUDE="" \
		DEFINES="-DWAIT3CODE -DBSD44 -DNAMEDPIPES \
				-DNO_ISO -Dsetpgid=setpgrp" \
		OPT="-O" LIBS="-lcompat"

#ULTRIX definitions
ultrix4.0 ultrix4.1 ultrix4.2 ultrix4.3:
	$(MAKE) objs \
		INCLUDE="-I." \
		DEFINES="-DWAIT3CODE -DNO_ISO -DNAMEDPIPES" \
		LIBS="" OPT="-O"

sysV:
	$(MAKE) objs DEFINES="-DSYSV -DSIGCHLD=SIGCLD -DNO_ISO" \
		INCLUDE="-I/usr/win-include" OPT="-O" LIBS="-lnet"

motorola:
	$(MAKE) objs \
		DEFINES="-DSYSV -DSIGCHLD=SIGCLD" \
		INCLUDE="" OPT="-O" LIBS="-linet"

CFLAGS=${OPT} ${INCLUDE} ${DEFINES}

objs: nettestd nettest

nettestd: nettestd.c nettest.h
	${CC} ${CFLAGS} nettestd.c -o nettestd ${LIBS}

nettest: nettest.c nettest.h
	${CC} ${CFLAGS} nettest.c -o nettest ${LIBS}

clean:
	rm -f core *.o a.out

clobber: clean
	rm -f nettest nettestd

tar:
	dir=`date | sed \
	 -e 's/....\(...\).\(..\)................\(..\)/nettest.\3.\1.\2/'\
	    -e 's/ 1/01/' -e 's/ 2/02/' -e 's/ 3/03/' -e 's/ 4/04/' \
	    -e 's/ 5/05/' -e 's/ 6/06/' -e 's/ 7/07/' -e 's/ 8/08/' \
	    -e 's/ 9/09/' \
	    -e 's/Jan/01/' -e 's/Feb/02/' -e 's/Mar/03/' -e 's/Apr/04/' \
	    -e 's/May/05/' -e 's/Jun/06/' -e 's/Jul/07/' -e 's/Aug/08/' \
	    -e 's/Sep/09/' -e 's/Oct/10/' -e 's/Nov/11/' -e 's/Dec/12/'`; \
	tar -crf - ${FILES} | compress >$$dir.tar.Z

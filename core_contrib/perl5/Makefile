#	BSDI	$Id$

.if ${MACHINE} == "sparc_v9"
MCH=	sparc
.else
MCH=	${MACHINE}
.endif

NOMAN=	noman
NOOBJ=	noobj

PERL=	D

all:
	if [ ! -f ${PERL}/Makefile ]; then make depend; fi
	(cd ${PERL}; LD_LIBRARY_PATH=$$(pwd):$$LD_LIBRARY_PATH make)

obj objdir tags:

clean:
	if [ -f ${PERL}/Makefile ]; then \
	    (cd ${PERL}; make clobber); \
	fi

cleandir:
	if [ -f ${PERL}/Makefile ]; then \
	    (cd ${PERL}; make clobber); \
	fi

depend:
	(   cd ${PERL};							\
            if [ -f config.sh ]; then mv config.sh config.sh.save; fi;	\
	    : -E causes configure to Exit before extracting config.sh;	\
	    ./Configure -dEs -Dbsdos_distribution=defined;		\
	    : Now we customize config.sh;				\
	    cp config.sh config.sh.tmp;					\
	    uname="$$(uname -s) bsdi.com $$(uname -r)";			\
	    uname="$$uname :bsdos_distribution defined: ${MCH}";	\
	    uname=$$(echo "$$uname" | tr '[:upper:]' '[:lower:]');	\
	    uname=$$(echo "$$uname" | sed -e 's/^[^=]*=//' -e 's/\///g'); \
	    sed -e "s/^\(# Configured by:\).*/\1 support/"		\
		-e "s/^\(# Target system:\).*/\1 $$uname/"		\
		-e "s/^\(cf_by\)=.*/\1='support'/"			\
		-e "s/^\(cf_email\)=.*/\1='support@bsdi.com'/"		\
		-e "s/^\(perladmin\)=.*/\1='support@bsdi.com'/"		\
		-e "s/^\(myarchname\)=.*/\1='${MCH}-bsdos'/"		\
		-e "s/^\(mydomain\)=.*/\1='.bsdi.com'/"			\
		-e "s/^\(myhostname\)=.*/\1='bsdi'/"			\
		-e "s/^\(myuname\)=.*/\1='$$uname'/"			\
		    < config.sh.tmp > config.sh;			\
	    rm -f config.sh.tmp;					\
	    : and now extract the files and make depend;		\
	    ./Configure -S;						\
	    make depend;						\
	)

beforeinstall:
	(cd ${PERL}; LD_LIBRARY_PATH=$$(pwd):$$LD_LIBRARY_PATH 		\
		make install.perl)

afterinstall:
	(cd ${PERL}; sh ../postinstall.bsdos)

mansourceinstall:

.include <bsd.prog.mk>

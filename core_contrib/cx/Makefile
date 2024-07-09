#	BSDI Makefile,v 1.2 2002/10/28 22:40:56 prb Exp

all install clean cleandir depend obj objdir tags mansourceinstall:

.if defined(MACHINE) && defined(OSNAME)

.if (${MACHINE} == "i386" && ${OSNAME} == "BSD/OS")
all install clean cleandir depend obj objdir tags mansourceinstall:
	${MAKE} -f Makefile.cx TARG=ppc BUILDARCH=powerpc $@
.endif

.if (${MACHINE} == "powerpc" && ${OSNAME} == "Darwin")
PATH:=		/usr/local/cx/bin:${PATH}
.EXPORT:	PATH
all install clean cleandir depend obj objdir tags mansourceinstall:
	${MAKE} -f Makefile.cx TARG=i386 BUILDARCH=i386 $@
	${MAKE} -f Makefile.cx TARG=ppc BUILDARCH=powerpc $@
.endif

i386:
	${MAKE} -f Makefile.cx TARG=i386 BUILDARCH=i386

powerpc ppc:
	${MAKE} -f Makefile.cx TARG=ppc BUILDARCH=powerpc
	
.endif

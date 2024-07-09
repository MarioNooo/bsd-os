#	BSDI Makefile,v 2.5 2002/07/22 16:30:08 seebs Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

FILES=	bsd.doc.mk bsd.lib.mk bsd.man.mk bsd.prog.mk bsd.subdir.mk sys.mk \
	bsd.submake.mk i386.mk powerpc.mk x86-bsdos4.mk x86-bsdos5.mk \
	x86-win32.mk
NOOBJ=	noobj

all clean cleandir depend lint tags mansourceinstall:

install:
	install -c -o ${BINOWN} -g ${BINOWN} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/mk

.include <bsd.prog.mk>

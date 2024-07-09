#	BSDI Makefile,v 2.3 1997/11/05 01:38:10 torek Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	strip
SRCS=	strip.c strip_aout.c strip_elf32.c strip_elf64.c

install: maninstall
	install -c -o ${BINOWN} -g ${BINOWN} -m ${BINMODE} strip \
	    ${DESTDIR}${BINDIR}
	[ "X${STRIP}" != "X-s" ] || ./strip ${DESTDIR}${BINDIR}/strip
	[ "X${COPY}" = "X-c" ] || rm -f ./strip

.include <bsd.prog.mk>

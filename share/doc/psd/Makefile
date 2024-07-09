#	BSDI Makefile,v 2.4 1999/01/21 15:12:37 jch Exp
#
#	@(#)Makefile	8.2 (Berkeley) 5/23/94

# Missing:	01.cacm 02.implement 03.iosys 04.uprog 06.Clang 07.pascal
#		08.f77 09.f77io 11.adb 15.yacc 16.lex 14.sccs 17.m4
# Broken:	00.contents
#
# The following modules do not build/install:
# 10.gdb, 13.rcs 

BINDIR=	/usr/share/doc/psd
FILES=	Makefile Title
SUBDIR=	05.sysman 12.make 18.gprof 20.ipctut 21.ipc

Title.ps: ${FILES}
	groff Title > ${.TARGET}

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}

.include <bsd.subdir.mk>

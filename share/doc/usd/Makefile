#	BSDI	Makefile,v 2.3 1999/09/20 21:20:56 polk Exp
#	@(#)Makefile	8.3 (Berkeley) 5/16/94

# Missing:
# 01.begin 02.learn 03.shell 05.dc 06.bc 09.edtut 10.edadv 14.jove
# 15.sed 16.awk 17.msmacros 21.troff 22.trofftut 23.eqn 24.eqnguide
# 25.tbl 26.refer 27.invert 28.bib 29.diction

# The following modules do not build/install:
# 08.mh

BINDIR=	/usr/share/doc/usd
FILES=	Makefile Title
SUBDIR=	04.csh 07.mail \
	18.msdiffs 19.memacros 20.meref \
	30.rogue \
	31.trek

Title.ps: ${FILES}
	groff -ms Title > ${.TARGET}

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}

.include <bsd.subdir.mk>

#	BSDI Makefile,v 2.2 2003/07/08 21:56:07 polk Exp

.include "../Makefile.defs"

PROG=	troff
SRCS=   column.cc dictionary.cc div.cc env.cc input.cc majorminor.cc \
	node.cc number.cc reg.cc symbol.cc
LDADD+=	${LIBGROFF} ${LIBM}
DPADD+=	${LIBGROFF} ${LIBM}
CLEANFILES+=majorminor.cc

majorminor.cc: ${.CURDIR}/../VERSION
	@echo Making ${.TARGET}
	@echo const char \*major_version = \
	\"`sed -e 's/^\([^.]*\)\..*$$/\1/' ${.CURDIR}/../VERSION`\"\; >${.TARGET}
	@echo const char \*minor_version = \
	\"`sed -e 's/^[^.]*\.\([0-9]*\).*$$/\1/' ${.CURDIR}/../VERSION`\"\; >>${.TARGET}

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/hyphen.us \
		${DESTDIR}/${TMACDIR}/

.include "../Makefile.common"
.include <bsd.prog.mk>

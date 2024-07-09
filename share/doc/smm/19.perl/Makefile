#	BSDI	Makefile,v 2.1 1996/09/19 14:59:57 polk Exp

DIR=	smm/19.perl
SRCS=	perl.ms
MACROS=	-ms

paper.ps: ${SRCS}
	${SOELIM} ${SRCS} | ${TBL} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>

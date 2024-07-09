#       @(#)BSDI Makefile,v 2.2 2002/06/19 21:44:20 seebs Exp
default: abort

abort:
	@echo These documents are obsolete, and are no longer maintained.
	@echo Refer instead to the Programmer\'s Guide.

DIR=	bsdi
SRCS=	config.n driver.n
MACROS=	-pte -me

all:

.include <bsd.doc.mk>

fmt:	config.${PRINTER} driver.${PRINTER}

config.${PRINTER}: config.n
	${ROFF} config.n > ${.TARGET}

driver.${PRINTER}: driver.n
	${ROFF} driver.n > ${.TARGET}

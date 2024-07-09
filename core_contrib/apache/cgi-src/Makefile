#	BSDI	Makefile,v 1.4 2001/02/26 20:49:20 polk Exp
#
# Multiple program support Makefile for BSDI build environment

MAKEFILES= \
	change-passwd.mk	\
	imagemap.mk		\
	post-query.mk		\
	query.mk

all:
	@cd ${.CURDIR}; \
	for mkfile in ${MAKEFILES}; do \
	    ${MAKE} ${MFLAGS} -f $${mkfile} ${.TARGET}; \
	done

.DEFAULT:
	@cd ${.CURDIR}; \
	for mkfile in ${MAKEFILES}; do \
	    ${MAKE} ${MFLAGS} -f $${mkfile} ${.TARGET}; \
	done

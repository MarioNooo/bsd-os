
PROG=	setkey
SRCS=	setkey.c parse.y token.l
CFLAGS+=-g
LDADD+=	-ll -ly
DPADD+=	${LIBL} ${LIBY}
CLEANFILES+=	y.tab.c y.tab.h key_test.o keytest
YFLAGS+=-d

MAN8=	setkey.0

SCRIPTS=	scriptdump

BINOWN=root
BINGRP=bin
BINMODE=555

all: ${PROG} scriptdump ${MAN8}

CFLAGS+=-DIPSEC_DEBUG -DINET6 -DYY_NO_UNPUT
CFLAGS+=-I${.CURDIR} -I.
CFLAGS+=-I${.CURDIR}/../../lib/libipsec
LDADD+= -lpfkey -lipsec
CLEANFILES+=	scriptdump

LOCALPREFIX=	/usr

# bsdi does not respect .PATH on $>
scriptdump: scriptdump.pl
	sed -e 's#@LOCALPREFIX@#${LOCALPREFIX}#' < $> > scriptdump

.include <bsd.prog.mk>

CLEANFILES+=	y.tab.h

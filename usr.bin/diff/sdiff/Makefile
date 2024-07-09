#	BSDI:	Makefile,v 2.1 1995/02/03 09:39:18 polk Exp

.include "../../Makefile.inc"

PROG=	sdiff
CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR} -I${.CURDIR}/../diff
SRCS= 	sdiff.c getopt.c getopt1.c version.c

.PATH:	${.CURDIR}/../diff

.include <bsd.prog.mk>

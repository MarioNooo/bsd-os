#	BSDI:	Makefile,v 2.1 1995/02/03 09:35:46 polk Exp

.include "../../Makefile.inc"

PROG=	diff
CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR} -DPR_PROGRAM=\"/usr/bin/pr\"
SRCS= 	analyze.c cmpbuf.c context.c diff.c dir.c ed.c getopt.c \
	getopt1.c ifdef.c io.c normal.c regex.c side.c util.c version.c

.include <bsd.prog.mk>

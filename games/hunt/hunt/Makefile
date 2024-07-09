#	BSDI Makefile,v 2.2 1999/01/16 14:31:27 jch Exp
#
#  Hunt
#  Copyright (c) 1985 Conrad C. Huang, Gregory S. Couch, Kenneth C.R.C. Arnold
#  San Francisco, California
#
#  Copyright (c) 1985 Regents of the University of California.
#  All rights reserved.  The Berkeley software License Agreement
#  specifies the terms and conditions for redistribution.
#
# Game parameter flags are:
#	RANDOM	Include doors which disperse shots randomly
#	REFLECT	Include diagonal walls that reflect shots
#	MONITOR	Include code for watching the game from the sidelines
#	OOZE	Include slime shots
#	FLY	Make people fly when walls regenerate under them
#	VOLCANO	Include occasional large slime explosions
#	DRONE	Include occasional drone shots
#	BOOTS	Include boots (which makes you immune to slime)
#	OTTO	Reserved for CGL automatic player
#
PROG=	hunt
SRCS=	hunt.c connect.c playit.c pathname.c otto.c
MAN6=	hunt.0
HIDEGAME=hidegame

CFLAGS+=	-DRANDOM -DREFLECT -DMONITOR -DOOZE -DFLY -DBOOTS -DUSE_CURSES
CFLAGS+=	-DOTTO -DVOLCANO -DHUNTD=\"${LIBDIR}/huntd\"
CFLAGS+=	-DINTERNET -DLOG -DBSD_RELEASE=44 -DSIGNAL_TYPE=void
CFLAGS+=	-I. -I${.CURDIR} -I${.CURDIR}/../huntd
LDADD+=		-lcurses -ltermcap
DPADD+=		${LIBCURSES} ${LIBTERMCAP}

.include "../../Makefile.inc"

.include <bsd.prog.mk>

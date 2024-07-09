# BSDI	Makefile,v 2.1 1995/02/03 06:21:06 polk Exp
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
PROG=	huntd
SRCS=	answer.c draw.c driver.c execute.c expl.c extern.c makemaze.c \
	shots.c terminal.c pathname.c faketalk.c ctl.c ctl_transact.c \
	get_names.c
MAN6=	huntd.0

CFLAGS+=	-DRANDOM -DREFLECT -DMONITOR -DOOZE -DFLY -DBOOTS 
CFLAGS+=	-DOTTO -DVOLCANO -DHUNTD=\"${LIBDIR}/huntd\"
CFLAGS+=	-DINTERNET -DLOG -DBSD_RELEASE=44 -DSIGNAL_TYPE=void
CFLAGS+=	-I. -I${.CURDIR} 

.PATH: ${.CURDIR}/../hunt

.include "../../Makefile.inc"
BINDIR=${LIBDIR}

.include <bsd.prog.mk>

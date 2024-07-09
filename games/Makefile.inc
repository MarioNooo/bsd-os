#	BSDI	Makefile.inc,v 2.1 1995/02/03 05:57:59 polk Exp
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

BINOWN?=	games
.if defined(HIDEGAME)
BINDIR?=	/usr/games/hide
BINMODE?=	4700
.else
BINDIR?=	/usr/games
.endif
LIBDIR?=        /usr/games/lib

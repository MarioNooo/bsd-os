#	BSDI Makefile,v 2.1 1995/02/03 06:28:26 polk Exp
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	rogue
CFLAGS+=-DUNIX -fwritable-strings
SRCS=	curses.c hit.c init.c inventory.c level.c machdep.c main.c \
	message.c monster.c move.c object.c pack.c play.c random.c ring.c \
	room.c save.c score.c spec_hit.c throw.c trap.c use.c zap.c
DPADD=	${LIBCURSES} ${LIBTERM} ${LIBCOMPAT}
LDADD=	-lcurses -ltermcap -lcompat
HIDEGAME=hidegame
MAN6=	rogue.0

.include <bsd.prog.mk>

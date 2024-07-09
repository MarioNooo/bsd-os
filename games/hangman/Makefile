#	BSDI Makefile,v 2.1 1995/02/03 06:20:14 polk Exp
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	hangman
SRCS=	endgame.c extern.c getguess.c getword.c main.c playgame.c \
	prdata.c prman.c prword.c setup.c
MAN6=	hangman.0
DPADD=	${LIBCURSES} ${LIBTERM} ${LIBCOMPAT}
LDADD=	-lcurses -ltermcap -lcompat
HIDEGAME=hidegame

.include <bsd.prog.mk>



#	BSDI Makefile,v 2.2 1996/10/07 21:30:24 pjd Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

CFLAGS+= -g
PROG=	talk
DPADD=	${LIBCURSES} ${LIBTERMCAP} ${LIBCOMPAT}
LDADD=	-lcurses -ltermcap
SRCS=	ctl.c ctl_transact.c display.c get_addrs.c get_names.c \
	init_disp.c invite.c io.c look_up.c msgs.c talk.c

.include <bsd.prog.mk>

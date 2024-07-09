#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	init
MAN8=	init.0
DPADD=	${LIBUTIL}
LDADD=	-lutil -ldialer
BINMODE=500
CFLAGS+=-DDEBUGSHELL -DSECURE

.include <bsd.prog.mk>

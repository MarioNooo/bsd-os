#       BSDI Makefile,v 2.1 1995/02/03 13:22:50 polk Exp

PROG=   uuxqt
SRCS=   main.c do_uuxqt.c uuxqt_args.c
MAN8=	uuxqt.0
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

BINDIR= /usr/libexec
BINMODE=6110

.include <bsd.prog.mk>
.include <bsd.man.mk>

#	BSDI Makefile,v 2.1 1995/02/03 13:21:17 polk Exp

PROG=	uucico
SRCS=	main.c do_cico.c chksum.c cico_args.c f_proto.c g_proto.c \
	t_proto.c Connect.c Control.c Diallers.c Mesg.c Sequence.c \
	Status.c uu_termios.c
MAN8=	uucico.0
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

BINDIR=	/usr/libexec
SBINDIR=/usr/sbin
BINMODE=6111

LINKS=	${BINDIR}/uucico ${SBINDIR}/uucico

.include <bsd.prog.mk>
.include <bsd.man.mk>

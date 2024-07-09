#	BSDI Makefile,v 1.2 1998/12/15 21:24:13 polk Exp

PROG=	scspd
MAN8=	scspd.0
SRCS=	scsp_config_parse.y scsp_cafsm.c scsp_config.c scsp_config_lex.c
SRCS+=	scsp_hfsm.c scsp_if.c scsp_input.c scsp_log.c scsp_msg.c scsp_output.c
SRCS+=	scsp_print.c scsp_socket.c scsp_subr.c scsp_timer.c scspd.c

CFLAGS+=-I.

CLEANFILES+=	y.tab.h

.depend:	scsp_config_parse.o

.include <bsd.prog.mk>

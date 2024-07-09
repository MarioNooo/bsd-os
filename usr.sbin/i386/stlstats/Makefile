#   BSDI Makefile,v 1.2 1996/12/31 02:31:43 polk Exp

PROG=    stlstats
MAN8=    stlstats.0
CFLAGS+= -I/sys/i386/isa
LDADD=   -lcurses -ltermcap

.include <bsd.prog.mk>

stlstats.0: stlstats.8
	nroff -mdoc $? > $@

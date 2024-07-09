#	BSDI Makefile,v 2.3 1996/10/11 23:48:57 prb Exp
#	@(#)Makefile	5.9 (Berkeley) 5/11/90
#
# Files are:
#	/etc/remote		remote host description file
#	/etc/phones		phone number file, owned by ${OWNER} and
#				mode 6??
#
# Configuration defines:
#	BUFSIZ			buffer sizing from stdio, must be fed
#				explicitly to remcap.c if not 1024
#	CONNECT			enables ~C for running external protocols
#	DEFBR			default baud rate to make connection at
#	DEFFS			default frame size for FTP buffering of
#				writes on local side

PROG=	tip
CFLAGS+=-I${.CURDIR} ${DRIVERDEFS} -DCONNECT -DDEFBR=1200 -DDEFFS=BUFSIZ
DPADD=	${LIBUTIL}
LDADD=	-lutil -ldialer
LINKS=	${BINDIR}/tip ${BINDIR}/cu
MLINKS=	tip.1 cu.1
SRCS=	acu.c cmds.c cmdtab.c cu.c hunt.c partab.c \
	remote.c tip.c tipout.c uucplock.c value.c vars.c termctl.c ${DRIVERS}

BINOWN=	uucp
BINGRP=	dialer
BINMODE=4510

# -- remote.o depends on the Makefile because of DEFBR and DEFFS
remote.o: Makefile

.include <bsd.prog.mk>

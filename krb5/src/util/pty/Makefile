LIB=pty
CLEANFILES+=tags pty_err.h pty_err.c

LDDYNMAJOR=.1
LDDYNMINOR=.2

CFLAGS+= -I${.CURDIR}/../et -I${.CURDIR} -I.

CFLAGS+= -DHAVE_FCHMOD=1 -DHAVE_FCHOWN=1 -DHAVE_REVOKE=1 \
	-DHAVE_KILLPG=1 -DHAVE_OPENPTY=1 -DSTDC_HEADERS=1 -DHAVE_SETREUID=1 \
	-DHAVE_SETSID=1 -DHAVE_TTYNAME=1 -DHAVE_OPENPTY=1 -DHAVE_UNISTD_H=1 \
	-DHAVE_STRING_H=1 -DHAVE_SYS_FILIO_H=1 -DHAVE_SYS_SOCKIO_H=1 \
	-DHAVE_SYS_TTY_H=1 -DHAVE_TTYENT_H=1 -DHAVE_SYS_SELECT_H=1 \
	-DHAVE_SYS_WAIT_H=1 -DHAVE_WAITPID=1 -Dkrb5_sigtype=void \
	-DPOSIX_SIGTYPE=1 -DPOSIX_SETJMP=1 -DPOSIX_TERMIOS=1 -DHAVE_UTMP_H=1 \
	-DHAVE_STRUCT_UTMP_UT_HOST=1 -DHAVE_SETOWN=1 -DSETPGRP_TWOARG=1 

SRCS=	cleanup.c getpty.c init_slave.c open_ctty.c \
	open_slave.c update_utmp.c update_wtmp.c vhangup.c \
	void_assoc.c pty_err.c logwtmp.c init.c sane_hostname.c

.depend: pty_err.h pty_err.c

pty_err.h: pty_err.et
	awk -f ${.CURDIR}/../et/et_h.awk outfile=${.TARGET} ${.CURDIR}/pty_err.et

pty_err.c: pty_err.et
	awk -f ${.CURDIR}/../et/et_c.awk outfile=${.TARGET} ${.CURDIR}/pty_err.et

.include <bsd.lib.mk>

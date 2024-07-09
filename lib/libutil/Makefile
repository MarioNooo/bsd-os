#	BSDI Makefile,v 2.13 2002/01/18 21:06:33 polk Exp
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

LIB=	util
CFLAGS+=-I${.CURDIR}/passwd -I/sys
LDDYNDEP=-lc
PASSWD=	abort.c copy.c ent1.c enter.c init.c inplace.c install.c lock.c \
	misc.c next.c path.c rebuild.c split.c tmp.c unlimit.c unlock.c \
	update.c
SRCS=	check_expire.c disklabel.c dkcksum.c login.c login_tty.c logout.c \
	logwtmp.c pty.c setproctitle.c setterm.c ttysetdev.c \
	${PASSWD}

# For incompatible changes use increment major
# For incompatible changes use LDDYNMAJOR=.1 and reset LDDYNMINOR to .0
LDDYNMAJOR=.0
LDDYNMINOR=.0

.PATH: ${.CURDIR}/passwd

afterinstall:
	install -c -o ${BINOWN} -g ${BINOWN} -m 444 \
	    ${.CURDIR}/passwd/libpasswd.h ${DESTDIR}/usr/include

.include <bsd.lib.mk>

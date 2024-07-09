#	makefile,v 2.4 1998/03/23 18:23:27 tks Exp

PROG=	portmap
MAN8=	portmap.0

COPTS=	-I${.OBJDIR} -DCHECK_PORT -DFACILITY=LOG_DAEMON -DHAS_SA_LEN -DHAS_GETIFADDRS

SRCS =	${PROG}.c pmap_check.c from_local.c

CLEANFILES += uintslen.h gen_uintslen

.depend: uintslen.h

uintslen.h: gen_uintslen
	./gen_uintslen > uintslen.h

${PROG} : uintslen.h

.include <bsd.prog.mk>

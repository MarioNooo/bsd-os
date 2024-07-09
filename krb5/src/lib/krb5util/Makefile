LIB=krb5util
LDDYNMAJOR=.1
LDDYNMINOR=.0

SRCS =	compat_recv.c seteuid.c

CFLAGS+=-I. -I${.CURDIR} -I${DESTDIR}/usr/include/krb5 \
	-I${.CURDIR}/../../util/et \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj \
	-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/../../include/krb5
CFLAGS+=-DHAVE_UNISTD_H=1 -DHAVE_STDLIB_H=1 -DHAVE_SYS_TYPES_H=1 \
	-DHAVE_SETEUID=1 -DHAVE_SETREUID=1 -DHAVE_SETEGID=1 \
	-DHAVE_SETREGID=1 

.include <bsd.lib.mk>

LIB=	pfkey
CFLAGS+=-g
CFLAGS+=-DIPSEC_DEBUG -DIPSEC -DINET6 -I. -DYY_NO_UNPUT
CFLAGS+=-I${.CURDIR}/../../../../sys
CFLAGS+=-I${.CURDIR}/../../../../sys/i386

.PATH:	${.CURDIR}/../libipsec ${.CURDIR}/../../../../sys/netkey
SRCS=	pfkey.c pfkey_dump.c ipsec_strerror.c
SRCS+=	key_debug.c
#YPREFIX=__libyy
#LPREFIX=__libyy
#YHEADER=1

#MAN=	ipsec_set_policy.3 ipsec_strerror.3
#MLINKS+=ipsec_set_policy.3 ipsec_get_policylen.3 \
#	ipsec_set_policy.3 ipsec_dump_policy.3

NODYNAMIC=yes
NOPROFILE=yes
MKLINT=no

.include <bsd.lib.mk>

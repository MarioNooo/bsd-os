LIB=	ipsec
CFLAGS+=-g
CFLAGS+=-DIPSEC_DEBUG -DIPSEC -DINET6 -I. -I${.CURDIR}

.PATH:	${.CURDIR}/../../../../sys/netkey
#SRCS=	pfkey.c pfkey_dump.c
SRCS+=	ipsec_strerror.c policy_parse.c policy_token.c
SRCS+=	ipsec_get_policylen.c ipsec_dump_policy.c
#SRCS+=	key_debug.c
CLEANFILES+=	y.tab.c y.tab.h policy_parse.c  policy_token.c
YFLAGS+=	-d -p __libyy
LFLAGS+=	-P__libyy
LFLAGS+=	-olex.yy.c

MAN3=	ipsec_set_policy.0 ipsec_strerror.0
MLINKS= ipsec_set_policy.3 ipsec_get_policylen.3 \
	ipsec_set_policy.3 ipsec_dump_policy.3

NODYNAMIC=	yes

.include <bsd.lib.mk>

.depend: policy_parse.c policy_token.c

LIB=des425
LDDYNMAJOR=.3
LDDYNMINOR=.0

SRCS =	cksum.c des.c enc_dec.c key_parity.c key_sched.c new_rnd_key.c \
	pcbc_encrypt.c quad_cksum.c random_key.c read_passwd.c \
	str_to_key.c unix_time.c util.c weak_key.c k4_glue.c

CFLAGS+=-I. -I${.CURDIR} -I${DESTDIR}/usr/include/krb5 \
	-I${.CURDIR}/../crypto/des -I${.CURDIR}/../../util/et \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj \
	-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/../../include/krb5

CFLAGS+=-DSIZEOF_SHORT=2 -DSIZEOF_INT=4 -DSIZEOF_LONG=4 -DHAVE_PRAGMA_WEAK=1 


.include <bsd.lib.mk>

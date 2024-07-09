LIB=kadm5srv
LDDYNMAJOR=.5
LDDYNMINOR=.0

.PATH:	${.CURDIR}/..

SRCS =	svr_policy.c svr_principal.c server_acl.c server_kdb.c \
	server_misc.c server_init.c server_dict.c svr_iters.c \
	svr_chpass_util.c adb_xdr.c adb_policy.c adb_free.c \
	adb_openclose.c

.include "${.CURDIR}/../Makefile.common"

CFLAGS+=-I. -I${.CURDIR} -I${.CURDIR}/.. -I${DESTDIR}/usr/include/krb5 \
	-I${.CURDIR}/../../../include/krb5 -I${.CURDIR}/../.. \
	-I${.CURDIR}/../../../include -I${.CURDIR}/../../../include/obj \
	-I${.CURDIR}/../../../util/et -I${.CURDIR}/../../../util/profile \
	-I${.CURDIR}/../../../util/profile/obj -I${.CURDIR}/../../../util/dyn \
	-I${.CURDIR}/../../../util/db2 -I${.CURDIR}/../../../util/db2/include \
	-I${.CURDIR}/../../gssapi -I${.CURDIR}/../../gssapi/obj \
	-I${.CURDIR}/../../rpc -I${.CURDIR}/../../rpc/obj

.include <bsd.lib.mk>

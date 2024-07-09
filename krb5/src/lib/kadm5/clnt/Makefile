LIB=kadm5clnt
LDDYNMAJOR=.5
LDDYNMINOR=.0

.PATH:	${.CURDIR}/..

SRCS =	clnt_policy.c client_rpc.c client_principal.c client_init.c \
	clnt_privs.c clnt_chpass_util.c

.include "${.CURDIR}/../Makefile.common"

CFLAGS+=-I. -I${.CURDIR} -I${.CURDIR}/.. -I${DESTDIR}/usr/include/krb5 \
	-I${.CURDIR}/../../../include -I${.CURDIR}/../../../include/obj \
	-I${.CURDIR}/../../../include/krb5 -I${.CURDIR}/../../../util/et \
	-I${.CURDIR}/../../../util/profile \
	-I${.CURDIR}/../../../util/profile/obj \
	-I${.CURDIR}/../.. \
	-I${.CURDIR}/../../../util/db2 -I${.CURDIR}/../../../util/db2/include \
	-I${.CURDIR}/../../gssapi -I${.CURDIR}/../../gssapi/obj \
	-I${.CURDIR}/../../rpc -I${.CURDIR}/../../rpc/obj

.include <bsd.lib.mk>

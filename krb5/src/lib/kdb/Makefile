LIB=kdb5
LDDYNMAJOR=.3
LDDYNMINOR=.2

SRCS =	keytab.c encrypt_key.c decrypt_key.c kdb_cpw.c kdb_db2.c \
	kdb_xdr.c verify_mky.c fetch_mkey.c setup_mkey.c store_mkey.c

# make sure we get the correct db.h header!
CFLAGS+=-I. -I${.CURDIR} -I${DESTDIR}/usr/include/krb5 \
	-I${.CURDIR}/../../include/krb5 -I${.CURDIR}/../../util/et \
	-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj \
	-I${.CURDIR}/../../util/db2/include -I${.CURDIR}/../../util/db2

CFLAGS+=-DHAVE_TCL_H=1 -DHAVE_MEMMOVE=1

.include <bsd.lib.mk>

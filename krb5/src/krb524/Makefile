LIB=krb524
LDDYNMAJOR=.1
LDDYNMINOR=.0

CFLAGS+= -I${.CURDIR}/../util/et -I${.CURDIR} -I. \
	-I${DESTDIR}/usr/include/krb5 \
	-I${.CURDIR}/../include -I${.CURDIR}/../include/obj \
	-I${.CURDIR}/../include/krb5 -I${.CURDIR}/../include/kerberosIV \
	-I${.CURDIR}/../util/profile -I${.CURDIR}/../util/profile/obj

CFLAGS+= -DUSE_MASTER -DHAVE_SYS_SELECT_H=1 -DRETSIGTYPE=void

SRCS=	conv_creds.c conv_princ.c cnv_tkt_skey.c \
        encode.c misc.c globals.c sendmsg.c krb524_err.c

.depend: krb524_err.h krb524_err.c

CLEANFILES+=tags krb524_err.h krb524_err.c

krb524_err.h: krb524_err.et
	awk -f ${.CURDIR}/../util/et/et_h.awk outfile=${.TARGET} ${.ALLSRC}

krb524_err.c: krb524_err.et
	awk -f ${.CURDIR}/../util/et/et_c.awk outfile=${.TARGET} ${.ALLSRC}

.include <bsd.lib.mk>

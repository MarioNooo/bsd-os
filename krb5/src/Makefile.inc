.include "Makefile.conf"

.if defined(KERBEROS4)
CFLAGS+=	-DKRB5_KRB4_COMPAT
.endif

# defines based on how the package is configured
CFLAGS+=	-DKERBEROS \
		-DKRB5_DNS_LOOKUP_KDC \
		-DKRB5_DNS_LOOKUP_REALM \
		-DKRB5_DNS_LOOKUP

KRB5_INCDIR=	/usr/include/krb5

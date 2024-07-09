
PROG=	route6d
MAN8=	route6d.0

CFLAGS+=	-Dss_len=__ss_len -Dss_family=__ss_family -DADVAPI -DINET6 \
	-DHAVE_GETIFADDRS

.include <bsd.prog.mk>

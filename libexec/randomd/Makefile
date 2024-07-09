PROG=	randomd
MAN8=	randomd.0
CFLAGS+=	-DKRBDES
SRCS=	randomd.c md5c.c
LDADD+=	-lcrypto

.include <bsd.prog.mk>

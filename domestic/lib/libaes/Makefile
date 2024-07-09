LIB=	aes
CFLAGS=	-O3 -pedantic
SRCS=	rijndael-alg-fst.c rijndael-api-fst.c
MAN3+=	aes.0
MLINKS+=	aes.3 makeKey.3
MLINKS+=	aes.3 cipherInit.3
MLINKS+=	aes.3 blockEncrypt.3
MLINKS+=	aes.3 blockDecrypt.3

afterinstall:
	install -m 444 -c ${.CURDIR}/rijndael-api-fst.h ${DESTDIR}/usr/include/aes.h
.include <bsd.lib.mk>

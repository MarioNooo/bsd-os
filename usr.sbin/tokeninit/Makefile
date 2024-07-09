#	BSDI Makefile,v 1.2 2003/08/21 22:29:28 polk Exp

PROG=	tokeninit
SRCS=	tokeninit.c getepass.c init.c token.c tokendb.c

CFLAGS+=-I${.CURDIR}/../../libexec/login_token
.PATH:	${.CURDIR}/../../libexec/login_token

LDADD+=	-lcrypto
TOKENS=	activ crypto snk

MAN8=	tokeninit.0
MLINKS=	tokeninit.8 activinit.8
MLINKS+=tokeninit.8 cryptoinit.8
MLINKS+=tokeninit.8 snkinit.8

BINDIR?=/usr/sbin
BINOWN= root
BINMODE=500

afterinstall:
	for i in ${TOKENS} ; do \
	    cd ${DESTDIR}/${BINDIR} && \
	    rm -f $${i}init && \
	    ln ${PROG} $${i}init ; \
	done

.include <bsd.prog.mk>

#	BSDI Makefile,v 1.2 2003/08/21 22:29:27 polk Exp

BINDIR= /usr/libexec

PROG=	login_token
SRCS=	login_token.c getepass.c init.c token.c tokendb.c

LDADD+=	-lcrypto
TOKENS=	activ crypto snk

MAN8=	login_token.0
MLINKS=	login_token.8 login_activ.8
MLINKS+=login_token.8 login_crypto.8
MLINKS+=login_token.8 login_snk.8

afterinstall:
	for i in ${TOKENS} ; do \
	    cd ${DESTDIR}/${BINDIR} && \
	    rm -f login_$$i && \
	    ln ${PROG} login_$$i ; \
	done

BINOWN= root
BINMODE=4555

.include <bsd.prog.mk>


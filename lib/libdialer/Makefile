#	BSDI Makefile,v 1.5 2002/01/18 21:06:33 polk Exp
LIB=	dialer
SRCS=	argv.c cap.c connect.c getttydesc.c reset.c sendrecvfd.c status.c
NOSTATIC=nostatic
LDDYNDEP=-lc
HDRS=	libdialer.h
INCDIR=	${DESTDIR}/usr/include
CFLAGS+=-Wall -I${.CURDIR}

# For compatible changes, increment minor
# For incompatible changes use increment major
LDDYNMAJOR=.0
LDDYNMINOR=.0

afterinstall:
	if [ ! -d ${INCDIR} ] ; then \
	    mkdir ${INCDIR} && \
	    chmod 755 ${INCDIR} && \
	    chown ${BINOWN}.${BINOWN} ${INCDIR} ; \
	fi
	cd ${.CURDIR} && \
	    install -c -o ${BINOWN} -g ${BINOWN} -m 444 ${HDRS} ${INCDIR}
	
.include <bsd.lib.mk>

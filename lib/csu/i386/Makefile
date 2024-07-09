#	BSDI Makefile,v 2.5 2002/10/29 06:30:50 prb Exp

SRCS=	crt1.c crti.s crtn.s
OBJS=	crt1.o gcrt1.o crti.o crtn.o
.if defined(RELEASE)
CFLAGS+=-DRELEASE='"'"${RELEASE}"'"'
.else
CFLAGS+=-DRELEASE='"'"$$(uname -r)"'"'
.endif

all: ${OBJS}

gcrt1.o: crt1.c
	${CC} ${CFLAGS} -DGCRT1 -c -o ${.TARGET} ${.ALLSRC}

depend: .depend

.depend: ${SRCS}
	${MKDEP} ${CFLAGS:M-[ID]*} ${.ALLSRC:M*.c}

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

.include <bsd.prog.mk>

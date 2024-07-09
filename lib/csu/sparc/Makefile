#	BSDI Makefile,v 2.4 1998/08/27 17:48:38 torek Exp

SRCS=	crt1.c crti.s crtn.s
OBJS=	crt1.o gcrt1.o crti.o crtn.o
CFLAGS+=-DRELEASE='"'"$$(uname -r)"'"'

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

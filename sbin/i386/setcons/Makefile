#	BSDI Makefile,v 1.3 1999/02/11 17:06:50 bostic Exp

PROG=	setcons
SRCS=	bell.c console.c repeat.c keymap.c
CFLAGS+= -I/sys
MAN8=	setcons.0

KEYDIR=	/sys/i386/isa/kbd

beforeinstall: mkct.c
	@for i in `ls ${KEYDIR}/pcconstab.* | sed -e 's/.*pcconstab\.//'`;\
	do								\
		echo "$$i...";						\
		${CC} ${CFLAGS} -o mkct -DKBD=$$i ${.CURDIR}/mkct.c;	\
		./mkct;							\
		install -o bin -g bin -m 444 $$i ${DESTDIR}/usr/libdata/keyboard/$$i; \
		rm -f mkct;						\
	done

.include <bsd.prog.mk>

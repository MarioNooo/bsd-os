#	BSDI Makefile,v 2.1 1995/02/03 12:27:40 polk Exp

.include "../Makefile.defs"

DEV=	devX75-12
DEVFILES=DESC TR TI TB TBI CR CI CB CBI HR HI HB HBI NR NI NB NBI S

obj objdir tags depend all clean cleandir:

install:
	for f in ${DEVFILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			$$f ${DESTDIR}${FONTDIR}/${DEV}/; \
	done

.include <bsd.prog.mk>

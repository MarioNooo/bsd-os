#	BSDI Makefile,v 1.1 2000/03/08 22:28:51 polk Exp

.include "../Makefile.defs"

DEV=	devhtml
DEVFILES=DESC TR TI TB TBI CR CI CB CBI HR HI HB HBI NR NI NB NBI S

obj objdir tags depend all clean cleandir:

install:
	for f in ${DEVFILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			$$f ${DESTDIR}${FONTDIR}/${DEV}/; \
	done

.include <bsd.prog.mk>

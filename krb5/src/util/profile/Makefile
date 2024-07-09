LIB=profile
CLEANFILES+=tags profile.h prof_err.h prof_err.c

LDDYNMAJOR=.1
LDDYNMINOR=.1

CFLAGS+= -I${.CURDIR}/../et -I${.CURDIR} -I.

CFLAGS+= -DSIZEOF_SHORT=2 -DSIZEOF_INT=4 -DSIZEOF_LONG=4 \
	-DHAVE_UNISTD_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STAT=1 -DHAVE_ACCESS=1

SRCS=	prof_tree.c prof_file.c prof_parse.c prof_get.c prof_set.c \
        prof_err.c prof_init.c

.depend: profile.h prof_err.h prof_err.c

prof_err.h: prof_err.et
	awk -f ${.CURDIR}/../et/et_h.awk outfile=${.TARGET} ${.CURDIR}/prof_err.et

prof_err.c: prof_err.et
	awk -f ${.CURDIR}/../et/et_c.awk outfile=${.TARGET} ${.CURDIR}/prof_err.et

profile.h: prof_err.h ${.CURDIR}/profile.hin
	cat ${.ALLSRC} > ${.TARGET}

beforeinstall:
	if [ ! -d ${DESTDIR}/usr/include/krb5 ]; then \
		mkdir -p ${DESTDIR}/usr/include/krb5; \
	fi
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		profile.h ${DESTDIR}/usr/include/krb5

.include <bsd.lib.mk>

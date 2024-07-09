#	BSDI Makefile,v 2.4 2002/01/09 17:12:23 polk Exp

DIRFILE=${DESTDIR}/usr/share/info/dir
INSTINFO= \
	install-info --dir-file=${DIRFILE} --quiet --section='User tools'

INFO=diff.info diff.info-1 diff.info-2 diff.info-3 diff.info-4

obj objdir clean cleandir depend:

beforeinstall:
	-[ -f ${DIRFILE} ] || \
		${MINSTALL} ${.CURDIR}/../../../core_contrib/dir ${DIRFILE}
	${INSTINFO} --item='* Diff: (diff).			File comparison utilities.' diff.info
	for f in ${INFO}; do \
		install -c -m ${MANMODE} -o ${MANOWN} -g ${MANGRP} \
			${.CURDIR}/$$f ${DESTDIR}/usr/share/info; \
	done

.include <bsd.prog.mk>

#	BSDI makefile,v 1.3 2002/01/09 22:04:02 polk Exp

MAN1!=ls *.1 | sed 's/.1/.0/g'

DIRFILE=${DESTDIR}/usr/share/info/dir
INSTINFO= \
	install-info --dir-file=${DIRFILE} --quiet --section='User tools'

afterinstall:
	${MINSTALL} dc.info ${DESTDIR}/usr/share/info
	-[ -f ${DIRFILE} ] || \
		${MINSTALL} ${.CURDIR}/../../../core_contrib/dir ${DIRFILE}
	${INSTINFO} --item='* Dc: (dc).			GNU desk calculator.' dc.info

distclean:
	make -f Makefile ${MAKEFLAGS} distclean
	rm *.0

.include <bsd.prog.mk>

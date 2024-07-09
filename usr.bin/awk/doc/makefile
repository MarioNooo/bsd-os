#	BSDI makefile,v 1.3 2002/01/09 17:11:47 polk Exp

MAN1!=ls *.1 | sed 's/.1/.0/g'
MLINKS=gawk.1 awk.1
DIRFILE=${DESTDIR}/usr/share/info/dir
INSTINFO= \
	install-info --dir-file=${DIRFILE} --quiet --section='User tools'

afterinstall:
	-[ -f ${DIRFILE} ] || \
		${MINSTALL} ${.CURDIR}/../../../core_contrib/dir ${DIRFILE}
	${INSTINFO} gawk.info; \
	${MINSTALL} gawk.info ${DESTDIR}/usr/share/info

obj objdir depend:

distclean:
	make -f Makefile ${MAKEFLAGS} distclean
	rm *.0

.include <bsd.prog.mk>

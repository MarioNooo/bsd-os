# BSDI Makefile,v 2.3 1996/04/08 20:04:56 bostic Exp

SUBDIR= rundos kernel

.include "Makefile.inc" 

afterinstall:
	install -c rombios ${DESTDIR}${ROMBIOS_FILE_NAME}

.include <bsd.subdir.mk>

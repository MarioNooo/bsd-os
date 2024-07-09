#	BSDI Makefile,v 2.5 1996/11/05 01:46:15 polk Exp

PROG=	sco
SUBDIR=	syscalls emulator bin etc shlib include

BINDIR?=	/usr/bin

SCODIRS=	/sco /sco/bin /sco/dev /sco/etc /sco/etc/perms /sco/shlib \
		/sco/usr /sco/usr/lib /sco/usr/lib/sco /sco/usr/lib/terminfo
beforeinstall:
	-for d in ${SCODIRS}; do \
		mkdir -p ${DESTDIR}$$d; \
		chown root.wheel ${DESTDIR}$$d; \
		chmod 0755 ${DESTDIR}$$d; \
	done
	-umask 22; \
	for m in cat1 cat3 cat5; do \
		mkdir -p ${DESTDIR}/usr/share/man/$$m; \
	done

# don't write over an existing filemap file
# the fake PostScript printer makes Lotus 1-2-3 happy
afterinstall:
	-if [ ! -f ${DESTDIR}/sco/filemap ]; then \
		install -c -o root -g ${BINGRP} -m 0444 \
			${.CURDIR}/filemap ${DESTDIR}/sco/filemap; \
	fi
	-if [ ! -f ${DESTDIR}/sco/dev/socksys ]; then \
		install -c -o root -g ${BINGRP} -m 0444 \
			${.CURDIR}/dev/socksys ${DESTDIR}/sco/dev/socksys; \
	fi
	-if [ ! -f ${DESTDIR}/sco/usr/spool/lp ]; then \
		umask 022; mkdir -p \
		    ${DESTDIR}/sco/usr/spool/lp/admins/lp/printers; \
		umask 022; touch \
		    ${DESTDIR}/sco/usr/spool/lp/admins/lp/printers/PostScript; \
	fi
	-if [ ! -f ${DESTDIR}/sco/usr/lib/terminfo/i/ibmpc3 ]; then \
		umask 022; mkdir -p ${DESTDIR}/sco/usr/lib/terminfo/i \
			${DESTDIR}/sco/usr/lib/terminfo/x; \
		chown -R ${BINOWN}.${BINGRP} ${DESTDIR}/sco/usr/lib/terminfo; \
		install -c -o ${BINOWN} -g ${BINGRP} -m 0444 \
			${.CURDIR}/usr/lib/terminfo/i/ibmpc3 \
			${DESTDIR}/sco/usr/lib/terminfo/i/ibmpc3; \
		install -c -o ${BINOWN} -g ${BINGRP} -m 0444 \
			${.CURDIR}/usr/lib/terminfo/x/xterm \
			${DESTDIR}/sco/usr/lib/terminfo/x/xterm; \
	fi

.include <bsd.prog.mk>

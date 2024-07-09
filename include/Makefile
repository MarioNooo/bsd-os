#	BSDI Makefile,v 2.32 2003/06/17 00:10:30 torek Exp
#	@(#)Makefile	8.2 (Berkeley) 1/4/94
#
# Doing a make install builds /usr/include
#
# The ``rm -rf''s used below are safe because rm doesn't follow symbolic
# links.
#
all clean cleandir depend lint tags:

# Missing: mp.h

FILES=  a.out.h ar.h assert.h bitstring.h bsd_auth.h cdrom.h ctype.h db.h \
	dirent.h disktab.h dlfcn.h err.h fnmatch.h fstab.h fts.h glob.h \
	grp.h ieeefp.h ifaddrs.h iso646.h kvm.h limits.h locale.h \
	login_cap.h malloc.h memory.h mpool.h ndbm.h nlist.h paths.h \
	pthread.h pthread_var.h pwd.h ranlib.h regex.h regexp.h rune.h \
	runetype.h sched.h semaphore.h setjmp.h sgtty.h signal.h stab.h \
	stdbool.h stddef.h stdio.h stdlib.h string.h strings.h struct.h \
	sync.h sysexits.h tar.h time.h ttyent.h tzfile.h unistd.h utime.h \
	utmp.h values.h vis.h wchar.h

MFILES=	float.h frame.h stdarg.h varargs.h
LFILES=	elf.h errno.h fcntl.h syslog.h termios.h

DIRS=	arpa protocols
LDIRS=	dev isofs miscfs msdosfs net netinet netinet6 netkey \
	netsec nfs sys ufs vm
#LDIRS+=netccitt netiso netns

NOOBJ=	noobj

SYS=	/sys

# Define SHARED to indicate whether you want symbolic links to the system
# source (``symlinks''), or a separate copy (``copies''); (latter useful
# in environments where it's not possible to keep /sys publicly readable)
SHARED=	copies
# SHARED=	symlinks

install: ${SHARED} special
	@echo installing ${FILES}
	@-for i in ${FILES}; do \
		cmp -s $$i ${DESTDIR}/usr/include/$$i > /dev/null 2>&1 || \
		    install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		    ${DESTDIR}/usr/include/$$i; \
	done
	@echo installing ${DIRS}
	@-for i in ${DIRS}; do \
		if [ ! -d ${DESTDIR}/usr/include/$$i ]; \
		then \
			mkdir ${DESTDIR}/usr/include/$$i; \
		fi; \
		chown ${BINOWN}.${BINGRP} ${DESTDIR}/usr/include/$$i; \
		chmod 755 ${DESTDIR}/usr/include/$$i; \
		(cd $$i; for j in *.[ih]; do \
			cmp -s $$j ${DESTDIR}/usr/include/$$i/$$j > \
			    /dev/null 2>&1 || \
			    install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			    ${DESTDIR}/usr/include/$$i/$$j; \
		done); \
	done
	@echo installing ${LFILES}
	@-for i in ${LFILES}; do \
		rm -f ${DESTDIR}/usr/include/$$i; \
		ln -s sys/$$i ${DESTDIR}/usr/include/$$i; \
		chown ${BINOWN}.${BINGRP} ${DESTDIR}/usr/include/$$i; \
	done
	@echo installing ${MFILES}
	@-for i in ${MFILES}; do \
		rm -f ${DESTDIR}/usr/include/$$i; \
		ln -s machine/$$i ${DESTDIR}/usr/include/$$i; \
		chown ${BINOWN}.${BINGRP} ${DESTDIR}/usr/include/$$i; \
	done

copies:
	@echo copies: ${LDIRS}
	@mkdir -p ${DESTDIR}/usr/include
	@-for i in ${LDIRS}; do \
		rm -rf ${DESTDIR}/usr/include/$$i; \
		if [ ! -d ${SYS}/$$i ]; \
		then \
			continue; \
		fi; \
		cd ${SYS} && \
		pax -rwH `find $$i -name \*.h -print` ${DESTDIR}/usr/include; \
		chown -R ${BINOWN}.${BINGRP} ${DESTDIR}/usr/include/$$i; \
		find ${DESTDIR}/usr/include/$$i -type f -print | \
			xargs chmod 444 ; \
		find ${DESTDIR}/usr/include/$$i -type d -print | \
			xargs chmod 755 ; \
	done
	rm -rf ${DESTDIR}/usr/include/machine
	mkdir ${DESTDIR}/usr/include/machine
	cd ${SYS}/${MACHINE}/machine && \
		pax -rwH *.h ${DESTDIR}/usr/include/machine; \
	chown -R ${BINOWN}.${BINGRP} ${DESTDIR}/usr/include/machine; \
	chmod -R 444 ${DESTDIR}/usr/include/machine; \
	chmod 755 ${DESTDIR}/usr/include/machine; \

symlink symlinks:
	@echo symlinks: ${LDIRS}
	@for i in ${LDIRS}; do \
		rm -rf ${DESTDIR}/usr/include/$$i; \
		ln -s ${SYS}/$$i ${DESTDIR}/usr/include/$$i; \
	done
	rm -rf ${DESTDIR}/usr/include/machine
	ln -s ${DESTDIR}${SYS}/${MACHINE}/machine ${DESTDIR}/usr/include/machine

special:

.include <bsd.prog.mk>

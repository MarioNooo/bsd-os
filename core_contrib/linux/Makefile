#	BSDI Makefile,v 1.9 2000/04/14 03:11:49 donn Exp

SUBDIR=	afdb transform ld-1.9.5.so ld-2.1.3.so liblinux.so.1 liblinux.so.2

TARBALL=${.CURDIR}/linux.lib.tar.gz

beforeinstall:
	ln -fs usr/linux /linux
	ln -fs ../linux/etc/nsswitch.conf /etc/
	ln -fs ../linux/lib/ld-linux.so.1 /lib/
	ln -fs ../linux/lib/ld-linux.so.2 /lib/
	gzcat ${TARBALL} | (cd / ; pax -rvpe)

LAP=	/usr/linux /linux /lib /linux/etc /linux/bin \
	/etc/ld.so.preload /etc/nsswitch.conf \
	/lib/ld-linux.so.1 /lib/ld-linux.so.2 \
	/linux/bin/ls /linux/bin/lsf /linux/bin/sh /linux/bin/uname \
	/linux/etc/ld.so.cache /linux/etc/ld.so.cache.1 /linux/etc/ld.so.conf \
	/linux/etc/ld.so.conf.1 /linux/etc/ld.so.preload \
	/linux/etc/ld.so.preload.1 /linux/etc/nsswitch.conf \
	/linux/lib/ld-1.9.5.so /linux/lib/ld-linux.so.1 \
	/linux/lib/ld-linux.so.1.9.5 \
	/linux/usr/i486-linux-libc5/lib/libdl.so.1 \
	/linux/usr/i486-linux-libc5/lib/liblinux.so.1 \
	/linux/lib/ld-2.0.7.so /linux/lib/ld-linux.so.2 \
	/linux/lib/liblinux.so.2

NOTLAP=	-s,.*/linux/binaries.*,, \
	-s,.*/linux/bin.*,, \
	${LAP:S/^/-s,.*/g:S/$/\$,,/g} \
	-s,/usr,, /usr/linux

LAP:
	pax -wd ${LAP} | gzip > LAP.tar.gz

LINUX:
	pax -w ${NOTLAP} | gzip > LINUX.tar.gz


release:	LAP LINUX
#	pax -w /linux /usr/linux /lib /etc/ld.so.preload /etc/nsswitch.conf | \
#		gzip > ${.CURDIR}/release.tar.gz

.include <bsd.subdir.mk>

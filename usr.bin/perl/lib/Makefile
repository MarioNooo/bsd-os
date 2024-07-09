#  BSDI Makefile,v 1.5 1995/08/03 00:58:35 polk Exp 

PERLLIB=abbrev.pl assert.pl bigfloat.pl bigint.pl bigrat.pl cacheout.pl \
	chat2.pl complete.pl ctime.pl dumpvar.pl exceptions.pl fastcwd.pl \
	find.pl finddepth.pl flush.pl getcwd.pl getopt.pl getopts.pl \
	importenv.pl ioctl.pl look.pl newgetopt.pl perldb.pl pwd.pl \
	shellwords.pl stat.pl syslog.pl termcap.pl timelocal.pl validate.pl \
	open2.pl
SIZEOF=	/usr/libdata/perl/sys/sizeof.ph

all:	

clean depend lint tags cleandir:

install:
	for plib in ${PERLLIB}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
			${.CURDIR}/$$plib \
			${DESTDIR}/usr/libdata/perl/$$plib ;\
	done
	${.CURDIR}/getioctlsizes | ${.CURDIR}/mksizes > \
		${DESTDIR}${SIZEOF}
	chown ${BINOWN}.${BINGRP} ${DESTDIR}${SIZEOF}
	chmod ${BINMODE} ${DESTDIR}${SIZEOF}

.include <bsd.prog.mk>

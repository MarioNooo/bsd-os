#	BSDI	Makefile,v 2.49 2003/08/14 15:35:42 polk Exp
#	@(#)Makefile	5.11 (Berkeley) 5/21/91

.if ${MACHINE} == "sparc_v9"
MCH=	sparc
.else
MCH=	${MACHINE}
.endif

# utmp is group writable by UTMPGRP
UTMPGRP=utmp

# Make everything owned by root/wheel in general
BINOWN=root
BINGRP=wheel

NOOBJ=	oobj

# -rw-r--r--
BIN=    changelist csh.cshrc csh.login csh.logout crontab daily \
	daily.local dm.conf gettytab group hosts \
	inetd.conf inetd.conf.nowrappers irs.conf login.conf man.conf \
	mime.types monthly monthly.local motd netgroup \
	netstart.proto phones protocols rc rc.local remote \
	rpc security services shells syslog.conf weekly weekly.local \
	mtools.conf networks \
	etc.${MCH}/disktab etc.${MCH}/shlib.map etc.${MCH}/ttys

.if     exists(etc.${MCH}/pccard.conf)
BIN+=   etc.${MCH}/pccard.conf
.endif

RC_CONF!=find rc.configure -name '[0-9]*'
.if     exists(etc.${MCH}/rc.configure)
TMP!=find etc.${MCH}/rc.configure -name '[0-9]*'
RC_CONF:=${RC_CONF} ${TMP}
.endif

RC_HW!=find rc.hardware -name 'README*'
.if	exists(etc.${MCH}/rc.hardware)
TMP!=find etc.${MCH}/rc.hardware -name '[0-9]*' -o -name 'README.*' -o -name 'proto.*'
RC_HW:=${RC_HW} ${TMP}
.endif


.if	exists(etc.${MCH}/Makefile.inc)
.include "etc.${MCH}/Makefile.inc"
.endif

MTREE=	BSDI.contrib.dist BSDI.root.dist BSDI.usr.dist BSDI.var.dist
NAMEDB=	root.cache
RBOOTD=	rbootd.conf

# Radius configuration files (/etc/raddb)
RADDB=	servers.example
RADOWN=	radius
RADGRP=	radius

all clean cleandir depend etc install lint:

distribution:
	install -c -o ${BINOWN} -g ${BINGRP} -m 644 ${BIN} ${DESTDIR}/etc
	install -c -o ${BINOWN} -g ${BINGRP} -m 644 aliases \
		${DESTDIR}/etc/mail/aliases
	ln -sf mail/aliases ${DESTDIR}/etc/aliases
	install -c -o root -g wheel -m 600 master.passwd ${DESTDIR}/etc
	install -c -o root -g wheel -m 600 chap_md5_secrets ${DESTDIR}/etc
	install -c -o ${BINOWN} -g ${BINGRP} -m 600 hosts.equiv ${DESTDIR}/etc
	install -c -o ${BINOWN} -g ${BINGRP} -m 644 banned ${DESTDIR}/etc/ftpd
	ln -fs ftpd/banned ${DESTDIR}/etc/ftpusers
	install -c -o ${BINOWN} -g ${BINGRP} -m 555 \
	     MAKEDEV.local etc.${MCH}/MAKEDEV ${DESTDIR}/dev
	(cd root; \
		install -c -o root -g wheel -m 644 dot.cshrc \
		    ${DESTDIR}/root/.cshrc; \
		install -c -o root -g wheel -m 644 dot.login \
		    ${DESTDIR}/root/.login; \
		install -c -o root -g wheel -m 644 dot.profile \
		    ${DESTDIR}/root/.profile; \
		install -c -o root -g wheel -m 644 slash.dot.profile \
		    ${DESTDIR}/.profile; )
	cd mtree; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${MTREE} \
	    ${DESTDIR}/etc/mtree
	cd mtree; install -c -o ${BINOWN} -g ${BINGRP} -m 600 special \
	    ${DESTDIR}/etc/mtree
	cd namedb; install -c -o ${BINOWN} -g ${BINGRP} -m 644 ${NAMEDB} \
	    ${DESTDIR}/etc/namedb
	cd raddb; install -c -o ${RADOWN} -g ${RADGRP} -m 600 ${RADDB} \
	    ${DESTDIR}/etc/raddb
	install -c -o ${BINOWN} -g operator -m 664 /dev/null \
	    ${DESTDIR}/etc/dumpdates
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/messages
	install -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
	    ${DESTDIR}/var/log/secure
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/daemon.log
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/ftpd/ftp.log
	install -c -o news -g news -m 664 /dev/null \
	    ${DESTDIR}/var/log/news/news
	install -c -o news -g news -m 664 /dev/null \
	    ${DESTDIR}/var/log/news/news.err
	install -c -o news -g news -m 664 /dev/null \
	    ${DESTDIR}/var/log/news/news.crit
	install -c -o news -g news -m 664 /dev/null \
	    ${DESTDIR}/var/log/news/news.notice
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/maillog
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/lpd-errs
	install -c -o ${BINOWN} -g ${UTMPGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/run/utmp
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/cron
	install -c -o nobody -g nogroup -m 664 /dev/null \
	    ${DESTDIR}/var/db/locate.database
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/lastlog
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/wtmp
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/account/acct
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    etc.${MCH}/fstab.* ${DESTDIR}/etc
	(cd etc.${MCH}; install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    boot.define ${DESTDIR}/etc)
	install -c -o ${BINOWN} -g ${BINGRP} -m 555 \
	    ${RC_CONF} ${DESTDIR}/etc/rc.configure
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${RC_HW}  ${DESTDIR}/etc/rc.hardware
.if	exists(etc.${MCH}/ld.so.conf)
	install -c -o ${BINOWN} -g ${BINGRP} -m 644 etc.${MCH}/ld.so.conf \
	    ${DESTDIR}/etc
.endif
.include <bsd.prog.mk>

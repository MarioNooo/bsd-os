#	BSDI	Makefile,v 2.67 2003/08/06 14:49:46 polk Exp
#	@(#)Makefile	5.20 (Berkeley) 6/12/93

# missing: mkproto quot
SUBDIR= ac accton adduser addgroup arp chkprt chown chroot config config_X \
	config_anonftp config_www cron dev_mkdb diskpart edquota ftp-proxy \
	gettyconv gettyd gettystat inetd installfloppy installsw iostat \
	ipfw ipfwasm ipfwcache ipfwcircuit ipfwcmp ipfwdump \
	ipfwechochk ipfwflow ipfwlog ipfwnat ipfwrate ipfwthrottle ipfwrewrite \
	kgmon kvm_mkdb lptest makewhatis manzip \
	modlog mtree netstat portmap pppstat pstat pwd_mkdb quotaon \
	rarpd repquota rmt rotate rpasswdadm rstatd rwhod \
	sa sliplogin syslogd timed trpt vipw \
	vmstat ypbind ypset

.if   ${MACHINE} == "i386"
SUBDIR+=i386 shlib shlist tdump
.if exists(i386.ND)
SUBDIR+=i386.ND
.endif
.elif ${MACHINE} == "sparc" || ${MACHINE} == "sparc_v9"
SUBDIR+=atm sparc shlib shlist tdump
# XXX: maxim is not ready for sparcs yet
SUBDIR:=${SUBDIR:Nmaxim}
.elif ${MACHINE} == "powerpc"
SUBDIR+=
.if exists(powerpc.ND)
SUBDIR+=shlib shlist powerpc.ND
.endif
.endif

.if exists(${.CURDIR}/../domestic/Makefile)
SUBDIR+=tokenadm tokeninit
.endif

.include <bsd.subdir.mk>

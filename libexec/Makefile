#	BSDI Makefile,v 2.29 2003/08/21 18:43:41 polk Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

SUBDIR=	chap_md5 comsat configure_system crypt_passwd \
        diff3 fingerd ftpd getNAME getty ld.elf_so linkaddr \
	login_chpass login_lchpass login_passwd login_radius login_rchpass \
	login_reject login_rpasswd mail.local makekey mexpect poppassd \
	ppp_proxy randomd rexecd talkd tftpd statd lockd

.if exists(${.CURDIR}/../domestic/Makefile)
# The following require Kerberos to compile as configured
SUBDIR+=authsrv identd login_auth login_kerberos login_krb-or-pwd \
	login_token telnetd
.endif

.if	${MACHINE} == "hp300"
SUBDIR+=rbootd
.elif	${MACHINE} == "luna68k"
SUBDIR+=rbootd
.endif

.include <bsd.subdir.mk>

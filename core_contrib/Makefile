#	BSDI Makefile,v 1.19 2003/08/30 14:07:28 polk Exp

FIXME: cx dejagnu
SUBDIR= LPRng am-utils apache bash bind binutils bison cvs daemon \
	dhcp file gcc gdb gettext gm4 gmake gzip kame less \
	libtool md5.shs mrouted ncurses nettest ntp nvi patch perl5 \
	qpopper shar tcpd tcpdump texinfo traceroute ucd-snmp zebra zlib

# OpenSSL needs to be installed before things that use it
SUBDIR+= openssl mod_ssl openssh sendmail 

# These require KERBEROS to compile as configured...
.if exists(${.CURDIR}/../domestic/Makefile)
SUBDIR+=x99token
.endif

# NO GATED!
#.if exists(${.CURDIR}/gated-uni.ND)
.if exists(DOES_NOT_EXIST)
SUBDIR+=gated-uni.ND routed.gated
.else
SUBDIR+=routed
.endif

.if ${MACHINE} == "i386"
# FIXME: linux
SUBDIR+=apm apmd
.elif ${MACHINE} == "sparc" || ${MACHINE} == "sparc_v9"
SUBDIR+=ld.so-1.9.5
.endif

.include <bsd.subdir.mk>

#	BSDI Makefile.inc,v 2.20 2002/01/09 21:49:22 polk Exp
#
#	@(#)Makefile.inc	8.2 (Berkeley) 9/5/93

# machine-independent net sources
.PATH: ${.CURDIR}/${MACHINE}/net ${.CURDIR}/net

SRCS+=	getifaddrs.c if_indextoname.c if_nameindex.c \
	if_nametoindex.c in6addr_any.c isfdtype.c link_addr.c \
	net_data.c nrl_security.c rcmd.c recv.c \
	send.c sockatmark.c yplib.c ypxdr.c

# machine-dependent net sources
.include "${.CURDIR}/${MACHINE}/net/Makefile.inc"

MAN3+=	byteorder.0 getgrent.0 getgrouplist.0 getifaddrs.0 getprotoent.0 \
	getpwent.0 getservent.0 if_indextoname.0 inet.0 isfdtype.0 \
	link_addr.0 rcmd.0 sockatmark.0

#
# XXX Many of the man pages that should now really be part of bind
#     are still here because the bind versions weren't as good as ours
#     or were missing completely.  Some were actually moved from the old
#     irs subdir which is gone now.  This seems like the repository for misc.
#     network stuff that doesn't fit elsewhere at this point...
#  
#
MLINKS+=byteorder.3 htonl.3 byteorder.3 htons.3 byteorder.3 ntohl.3 \
	byteorder.3 ntohs.3
MLINKS+=getgrent.3 getgrnam.3 \
	getgrent.3 getgrgid.3 \
	getgrent.3 setgroupent.3 \
	getgrent.3 setgrent.3 \
	getgrent.3 endgrent.3 \
	getgrent.3 getgrent_r.3 \
	getgrent.3 getgrnam_r.3 \
	getgrent.3 getgruid_r.3 \
	getgrent.3 setgrent_r.3 \
	getgrent.3 endgrent_r.3
MLINKS+=getprotoent.3 getprotobynumber.3 \
	getprotoent.3 getprotobyname.3 \
	getprotoent.3 setprotoent.3 \
	getprotoent.3 endprotoent.3
MLINKS+=getpwent.3 getpwnam.3 \
	getpwent.3 getpwuid.3 \
	getpwent.3 setpassent.3 \
	getpwent.3 setpwent.3 \
	getpwent.3 endpwent.3 \
	getpwent.3 getpwent_r.3 \
	getpwent.3 getpwnam_r.3 \
	getpwent.3 getpwuid_r.3 \
	getpwent.3 setpassent_r.3 \
	getpwent.3 setpwent_r.3 \
	getpwent.3 endpwent_r.3
MLINKS+=if_indextoname.3 if_nametoindex.3 if_indextoname.3 if_nameindex.3 \
	if_indextoname.3 if_freenameindex.3
MLINKS+=inet.3 addr.3 inet.3 inet_addr.3 \
	inet.3 aton.3 inet.3 inet_aton.3 \
	inet.3 lnaof.3 inet.3 inet_lnaof.3 \
	inet.3 makeaddr.3 inet.3 inet_makeaddr.3 \
	inet.3 netof.3 inet.3 inet_netof.3 \
	inet.3 network.3 inet.3 inet_network.3 \
	inet.3 ntoa.3 inet.3 inet_ntoa.3 \
	inet.3 ntop.3 inet.3 inet_ntop.3 \
	inet.3 pton.3 inet.3 inet_pton.3
MLINKS+=link_addr.3 link_ntoa.3
MLINKS+=rcmd.3 rresvport.3 rcmd.3 ruserok.3

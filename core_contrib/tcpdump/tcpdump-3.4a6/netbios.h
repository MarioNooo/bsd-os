/*
 * NETBIOS protocol formats
 *
 * @(#) /master/core_contrib/tcpdump/tcpdump-3.4a6/netbios.h,v 1.1.1.1 1998/02/02 19:04:20 jch Exp
 */

struct p8022Hdr {
    u_char	dsap;
    u_char	ssap;
    u_char	flags;
};

#define	p8022Size	3		/* min 802.2 header size */

#define UI		0x03		/* 802.2 flags */


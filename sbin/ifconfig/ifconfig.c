/*	BSDI ifconfig.c,v 2.64 2003/05/14 22:12:23 dab Exp	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * vlan logic based on work by Bill Paul.
 *
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ifconfig.c	8.2 (Berkeley) 2/16/94";
#endif /* not lint */

/*
 * We would like to define IPV4CIDR, but that changes the format
 * of the output for inet addresses from something like:
 *	inet 127.0.0.1 netmask 127.0.0.1
 * to
 *	inet 127.0.0.1/8
 * and there are things like maxim and other configuration stuff
 * that parse ifconfig output.  They need to be updated before
 * we change the default output format.
 *
 * #define IPV4CIDR
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/if_ppp.h>
#include <net/if_pppioctl.h>
#include <net/if_aif.h>
#include <net/if_pif.h>
#include <net/if_vlan.h>
#include <net/if_802_11.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_var.h>
#include <arpa/inet.h>

#ifdef XNS
#define	NSIP
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <netdb.h>

#define EON
#ifdef ISO
#include <netiso/iso.h>
#include <netiso/iso_var.h>
#endif
#include <sys/protosw.h>

#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <ifaddrs.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Known address families */
struct afswitch {
	const	char *af_name;		/* Address family name */
	int	af_af;
	struct	sockaddr *af_stab[3];	/* Table of interface addresses */
#define ADDR	0
#define MASK	1
#define DSTADDR	2
	void	(*af_status) __P((struct ifaddrs *));
	void	(*af_getaddr) __P((struct afswitch *, char *, long));
	int	(*af_compare) __P((struct sockaddr *, struct sockaddr *));
	int	af_difaddr;		/* ioctl to delete an address */
	int	af_aifaddr;		/* ioctl to add a new address */
	struct	ifreq *af_ridreq;	/* ifreq structure for delete */
	struct	ifaliasreq *af_addreq;	/* ifalias structure for add */
};

struct llswitch {
	const	char *ll_name;
	u_int	ll_type;
};

struct cmd {
	const	char *c_name;		/* Argument name */
	u_int	c_parameter;		/* Flag to set or reset */
#define	NEXTARG		0xffffff	/* Next argv */
#define	NEXTARG2	0xfffffe	/* Next argv */
	u_int	c_flag;			/* Flag to check and set */
	u_int	c_flag2;		/* More flags to check and set */
	void	(*c_func) __P((struct afswitch *,
			       struct cmd *,
			       char *));
	int	c_af;			/* 0, AF not required, -1, AF */
					/* required, or must match */
					/* supplied AF */
	u_int	c_ltype;		/* used by media routines */
	void	(*c_func2) __P((struct afswitch *,
			       struct cmd *,
			       char *, char *));
};

struct pifflags_cmd {
	char	*pf_name;		/* name */
	int	pf_report;		/* should this be printed in status? */
	u_long	pf_reset;		/* bits to clear (mask) */
	u_long	pf_set;			/* bits to set */
};

struct aifflags_cmd {
	char	*af_name;		/* name */
	int	af_report;		/* should this be printed in status? */
	u_long	af_reset;		/* bits to clear (mask) */
	u_long	af_set;			/* bits to set */
};

struct ifcapflags_cmd {
	char	 *ifc_name;		/* name */
	u_int64_t ifc_reset;		/* bits to clear (mask) */
	u_int64_t ifc_set;		/* bits to set */
};

static struct	ifaliasreq addreq;	/* For adding addresses */
static struct	ifreq dlreq;		/* For configuring link layer */
static struct	ifaddrs *ifaddrs;	/* List of address info from getifaddrs() */
static struct	ifreq ridreq;		/* For removing old address */
static u_long	ifmetric;		/* Metric */
static u_long	ifmtu;			/* MTU */
static u_int	ifflags_set;		/* Flags to be set */
static u_int	ifflags_reset;		/* Flags to be reset */
static int	mflags_set;		/* Media flags to be set */
static int	mflags_clr;		/* Media flags to be reset */
static int	cur_mtype;		/* Current media type to set */
static u_int	cur_ltype;		/* Current media type to set */
					/* shadows dlreq.ifr_addr.sdl_type */
static int	minst;			/* Media instance */
static int	Lflag = 0;		/* Display IPv6 address lifetime */
static int	mflag = 0;		/* Display all media possiblities */
static int	s = -1;		/* for address changes */

/* INET */
static struct	in_aliasreq in_addreq;

/* INET6 */
static struct	in6_ifreq inet6_ridreq;
static struct	in6_aliasreq inet6_addreq;

#ifdef ISO
/* ISO */
static struct	iso_aliasreq iso_addreq;
static struct	iso_ifreq iso_ridreq;
static u_int	nsellength = 1;		/* Length of ISO selector */
#endif

/* PIF */
static struct	pifreq pifreq;
static char	*pifname_add;
static char	*pifname_rm;
static u_int	pifflags_set;		/* PIF flags to be set */
static u_int	pifflags_reset;		/* PIF flags to be reset */

/* AIF */
static struct	aifreq aifreq;
static char	*aifname_add;
static char	*aifname_rm;
static struct	sockaddr_storage aifaddr;
static u_int	aifflags_set;		/* AIF flags to be set */
static u_int	aifflags_reset;		/* AIF flags to be reset */
static u_int	aif_vid;
static u_char	aif_ether[6];
static char	aif_subname[IFNAMSIZ];

/* VLAN */
static struct 	vlanreq vreq;

/* 802.11 */
static int channel;
static char station[16];
static int stationlen;
static char ssid[8][33];
static int ssidlen[8];
static int wepenable;
static int weptxkey;
static int authmode;

/* Interface Capabilities */
static struct	 ifcapreq ifcapreq;
static u_int64_t ifcap_set;		/* Capabilities to be set */
static u_int64_t ifcap_reset;		/* Capabilities to be reset */

/*
 * We allow cards to have up to 4 sets of wep keys
 * Currently we know the aironet has 2 sets, the volatile and non-volatile
 */
#define	NWEPKEYS	4 * IEEE_802_11_NWEPKEYS
static char wepkey[NWEPKEYS][IEEE_802_11_MAXWEPKEYLEN];
static int wepkeylen[NWEPKEYS];

/* tunnel */
static struct sockaddr_storage tunnelsrc;
static struct sockaddr_storage tunneldst;

static union {
	struct sockaddr_dl dl;
	char size[256];		/* That ought to be big enough! */
} laddr;


/* Parameters set */
enum {
	PS_ADDR =	0x00000001, /* Address has been set */
	PS_DSTADDR =	0x00000002, /* Destination address has been set */
	PS_BROADADDR =	0x00000004, /* Broadcast address has been set */
	PS_NETMASK =	0x00000008, /* Network Mask has been set */
	PS_METRIC =	0x00000010, /* Metric has been set */
	PS_LINKTYPE =	0x00000020, /* Link type has been set */
	PS_IPDST =	0x00000040, /* IP Destination (for EON) has been set */
	PS_NSELLENGTH =	0x00000080, /* NSEL length has been set (AF_ISO) */
	PS_SNPAOFFSET =	0x00000100, /* SNPA offset has been set (AF_ISO) */
	PS_ADDRMOD =	0x00000200, /* An address is being modified */
	PS_ADD =	0x00000400, /* Add an address */
	PS_REMOVE =	0x00000800, /* Remove an address */
	PS_MODIFY =	0x00001000, /* Modify an address */
	PS_ALIAS =	0x00002000, /* Add or change and alias */
	PS_DELETE =	0x00004000, /* Delete an alias */
#define	PS_ADDRATTR	(PS_ADDR | PS_DSTADDR | PS_BROADADDR | \
			 PS_NETMASK | PS_NSELLENGTH | PS_SNPAOFFSET)
	PS_MEDIATYPE =	0x00008000, /* Change media type selection */
	PS_MEDIAFLAG =	0x00010000, /* Media flags modified */
	PS_MEDIAINST =	0x00020000, /* Media instance supplied */
	PS_MTU =	0x00040000, /* MTU has been set */
	PS_PREFIXLEN =	0x00080000, /* Prefixlen specified */
	PS_SCOPEID =	0x00100000, /* Scope ID specified */
	PS_VPARENT =	0x00200000, /* Set VLAN parent */
	PS_DVPARENT =	0x00400000, /* Unset VLAN parent */
	PS_VID =	0x00800000, /* Set VLAN ID */
	PS_ADDPIF = 	0x01000000, /* Add a PIF */
	PS_RMPIF = 	0x02000000, /* Delete a PIF */
	PS_ANYCAST =	0x04000000, /* Anycast address flag */
	PS_TENTATIVE =	0x08000000, /* Tentative address flag */
	PS_LADDR = 	0x10000000, /* Set link level address */
	PS_PLTIME =	0x40000000, /* Preferred lifetime */
	PS_VLTIME =	0x80000000, /* Valid lifetime */
} parms_set;

enum {
	PS2_ADDAIF = 	0x00000001, /* Add an AIF */
	PS2_RMAIF = 	0x00000002, /* Delete an AIF */
	PS2_AIFADDR =	0x00000004, /* Specify address for AIF map */
	PS2_RMAIFADDR =	0x00000008, /* Remove address from AIF map */
	PS2_RMAIFIF =	0x00000010, /* Remove interface from AIF map */
	PS2_AIFVID =	0x00000020, /* Specify VLAN ID for AIF map */
	PS2_AIFETHER =	0x00000040, /* Specify ether addr for AIF map */
	PS2_AIFIF =	0x00000080, /* Specify interface for AIF map */
	PS2_ADDRMAP =	0x00000100, /* Dump AIF address map */
	PS2_STATION = 	0x00000200, /* Set 802.11 station name */
	PS2_CHANNEL = 	0x00000400, /* Set 802.11 channel */
	PS2_SSID = 	0x00000800, /* Set 802.11 desired SSID */
	PS2_WEPKEY = 	0x00001000, /* Set 802.11 WEP key */
	PS2_WEPENABLE = 0x00002000, /* Set 802.11 WEP enable */
	PS2_WEPTXKEY = 	0x00004000, /* Set 802.11 WEP TX key index */
	PS2_AUTHMODE =  0x00010000, /* Set 802.11 Authentication Mode */

	PS2_TUNNEL =    0x00020000, /* set phys address for gif tunnel */
	PS2_DELETETUNNEL = 0x00040000, /* clear phys address for gif tunnel */
} parms_set2;

static void	Perror __P((const char *));
#ifdef ISO
static void	adjust_nsellength __P((struct afswitch *));
#endif
static void	setaifaddr __P((struct afswitch *, struct cmd *, char *));
static void	addaif __P((struct afswitch *, struct cmd *, char *));
static void	addpif __P((struct afswitch *, struct cmd *, char *));
static void	dopifflags __P((struct afswitch *, struct cmd *, char *));
static void	doaifflags __P((struct afswitch *, struct cmd *, char *));
static void	doifcapflags __P((struct afswitch *, struct cmd *, char *));
static void	dumpaifmap __P((char *));
static void	media_status_all __P((char *ifname));
static void	media_status_current __P((char *ifname));
static char    *media_display_word __P((int, int));
static void 	vlan_status __P((char *));
static void 	ieee80211_status __P((char *));
static struct	ifaddrs *findaddr __P((struct afswitch *, const char *,
    struct sockaddr *));
static void	ifcap_warn __P((u_int64_t));
static void	ifcap_status __P((char *));
static void	nonoarp __P((struct afswitch *, struct cmd *, char *));
static void	notrailers __P((struct afswitch *, struct cmd *, char *));
static int	numaddrs __P((const char *, int));
static struct	afswitch *parse_af __P((const char *));
#ifdef IPV4CIDR
static int	prefix_length __P((struct sockaddr *mask, int af));
#endif
static void	fprintb __P((FILE *, const char *, u_int, u_int, const char *));
#define		printb(label, v, bits) fprintb(stdout, label, v, 0, bits)
static void	removepif __P((struct afswitch *, struct cmd *, char *));
static void	removeaif __P((struct afswitch *, struct cmd *, char *));
static void	resetifflags __P((struct afswitch *, struct cmd *, char *));
static void	scrubaddrs __P((const char *));
static void	setaifvid __P((struct afswitch *, struct cmd *, char *));
static void	setaifether __P((struct afswitch *, struct cmd *, char *));
static void	setaifif __P((struct afswitch *, struct cmd *, char *));
static void	setifaddr __P((struct afswitch *, struct cmd *, char *));
static void	setifbroadaddr __P((struct afswitch *, struct cmd *, char *));
static void	setifdstaddr __P((struct afswitch *, struct cmd *, char *));
static void	setifflags __P((struct afswitch *, struct cmd *, char *));
static void	setifipdst __P((struct afswitch *, struct cmd *, char *));
static void	setifnetmask __P((struct afswitch *, struct cmd *, char *));
static void	setifmetric __P((struct afswitch *, struct cmd *, char *));
static void	setifmtu __P((struct afswitch *, struct cmd *, char *));
static void	setlinktype __P((struct afswitch *, struct cmd *, char *));
#ifdef ISO
static void	setnsellength __P((struct afswitch *, struct cmd *, char *));
#endif
static void	setmedia __P((struct afswitch *, struct cmd *, char *));
static void	setsnpaoffset __P((struct afswitch *, struct cmd *, char *));
static void	setvid __P((struct afswitch *, struct cmd *, char *));
static void	setvparent __P((struct afswitch *, struct cmd *, char *));
static void	setwepkey __P((struct afswitch *, struct cmd *, char *));
static void	setwepenable __P((struct afswitch *, struct cmd *, char *));
static void	setwepdisable __P((struct afswitch *, struct cmd *, char *));
static void	setweptxkey __P((struct afswitch *, struct cmd *, char *));
static void	setauthmode __P((struct afswitch *, struct cmd *, char *));
static void	setstation __P((struct afswitch *, struct cmd *, char *));
static void	setssid __P((struct afswitch *, struct cmd *, char *));
static void	setladdr __P((struct afswitch *, struct cmd *, char *));
static void	setchannel __P((struct afswitch *, struct cmd *, char *));

static void	status __P((const char *, struct afswitch *));
static void	usage __P((void));

#define	SA(x)	((struct sockaddr *)(x))

/* INET */
static void	in_getaddr __P((struct afswitch *, char *, long));
static void	in_status __P((struct ifaddrs *));
static int	in_compare __P((struct sockaddr *, struct sockaddr *));

#define SIN(x) ((struct sockaddr_in *)(x))

/* INET6 */
static int	inet6_compare __P((struct sockaddr *s1, struct sockaddr *s2));
static void	inet6_getaddr __P((struct afswitch *afp, char *addr,
    long which));
static int	prefixlen __P((void *, size_t));
void		in6_fillscopeid __P((struct sockaddr_in6 *));
static void	inet6_status __P((struct ifaddrs *ifa));
static char 	*sec2str __P((time_t));

/* KAME originals */
static void setifprefixlen __P((struct afswitch *, struct cmd *, char *));
static void setifscopeid __P((struct afswitch *, struct cmd *, char *));
static void setip6flags __P((struct afswitch *, struct cmd *, char *));
static void resetip6flags __P((struct afswitch *, struct cmd *, char *));
static void setip6flags __P((struct afswitch *, struct cmd *, char *));
static void resetip6flags __P((struct afswitch *, struct cmd *, char *));
static void setip6deprecated __P((struct afswitch *, struct cmd *, char *));
static void setip6lifetime __P((struct afswitch *, struct cmd *, char *));
static void setip6lifetime __P((struct afswitch *, struct cmd *, char *));
static void in6_setmask __P((struct afswitch *, int));

#define SIN6(x) ((struct sockaddr_in6 *)(x))

static void	inet6_status __P((struct ifaddrs *));
static int	inet6_compare __P((struct sockaddr *, struct sockaddr *));

/* ISO */
static void	iso_getaddr __P((struct afswitch *, char *, long));
static void	iso_status __P((struct ifaddrs *));
static int	iso_compare __P((struct sockaddr *, struct sockaddr *));

#define SISO(x) ((struct sockaddr_iso *)(x))

/* Link (physical) */
static void	link_status __P((struct ifaddrs *));
static void	pif_aif_status __P((struct ifaddrs *));
static void	installladdr __P((char *));
static struct afswitch *afs_link;	/* Pointer to the entry for AF_LINK */

#define	SDL(x) ((struct sockaddr_dl *)(x))

/* 802.11 */
static void	installauthmode __P((char *));
static void	installwepkey __P((char *));
static void	installwepenable __P((char *));
static void	installweptxkey __P((char *));
static void	installstation __P((char *));
static void	installssid __P((char *));
static void	installchannel __P((char *));

#ifdef XNS
/* XNS */
/*
 * XNS support liberally adapted from code written at the University of
 * Maryland principally by James O'Toole and Chris Torek.
 */

static void	xns_getaddr __P((struct afswitch *, char *, long));
static void	xns_status __P((struct ifaddrs *));
static int	xns_compare __P((struct sockaddr *, struct sockaddr *));

#define SNS(x) ((struct sockaddr_ns *) (x))
#endif

/* tunnel */
static void	settunnel __P((struct afswitch *, struct cmd *, char *, char *));
static void	tunnel_status __P((struct ifaddrs *));
static void	installtunnel __P((char *));
static void	deletetunnel __P((char *));

static struct cmd cmds[] = {
		/* Flags */
	{ "up",		IFF_UP,		0, 0,		setifflags },
	{ "down",	IFF_UP,		0, 0,		resetifflags },
	{ "trailers",	0,		0, 0,		notrailers },
	{ "-trailers",	0,		0, 0,		notrailers },
	{ "arp",	0,		0, 0,		nonoarp },
	{ "-arp",	0,		0, 0,		nonoarp },
	{ "debug",	IFF_DEBUG,	0, 0,		setifflags },
	{ "-debug",	IFF_DEBUG,	0, 0,		resetifflags },
	{ "link0",	IFF_LINK0,	0, 0,		setifflags } ,
	{ "-link0",	IFF_LINK0,	0, 0,		resetifflags } ,
	{ "link1",	IFF_LINK1,	0, 0,		setifflags } ,
	{ "-link1",	IFF_LINK1,	0, 0,		resetifflags } ,
	{ "link2",	IFF_LINK2,	0, 0,		setifflags } ,
	{ "-link2",	IFF_LINK2,	0, 0,		resetifflags } ,
	{ "metric",	NEXTARG,	PS_METRIC, 0,	setifmetric },
	{ "mtu",	NEXTARG,	PS_MTU, 0,	setifmtu },

	    /* Address attributes */
	    /* Generic */
	{ "alias",	0,		PS_ADDRMOD|PS_ALIAS, 0 },
	{ "-alias",	0,		PS_ADDRMOD|PS_ALIAS|PS_DELETE, 0 },
	{ "delete",	0,		PS_ADDRMOD|PS_ALIAS|PS_DELETE, 0 },

	{ "add",	NEXTARG,	PS_ADDRMOD|PS_ADD|PS_ADDR, 0,
	    setifaddr },
	{ "modify",	NEXTARG,	PS_ADDRMOD|PS_MODIFY|PS_ADDR, 0,
	    setifaddr },
	{ "remove",	NEXTARG,	PS_ADDRMOD|PS_REMOVE|PS_ADDR, 0,
	    setifaddr },
	{ "-remove",	0,		PS_ADDRMOD|PS_REMOVE, 0 },
	{ "set",	NEXTARG,	PS_ADDR, 0,
	    setifaddr },

	{ "broadcast",	NEXTARG,	PS_BROADADDR, 0, setifbroadaddr },
	{ "destination", NEXTARG,	PS_DSTADDR, 0,	setifdstaddr },
	{ "netmask",	NEXTARG,	PS_NETMASK, 0,	setifnetmask },

	    /* IPv6 specific */
	{ "prefixlen",	NEXTARG,	PS_PREFIXLEN, 0, setifprefixlen },
	{ "scopeid",	NEXTARG,	0, 0,		setifscopeid },
	{ "anycast",	IN6_IFF_ANYCAST, 0, 0,		setip6flags },
	{ "-anycast",	IN6_IFF_ANYCAST, 0, 0,		resetip6flags },
	{ "autoconf",	IN6_IFF_AUTOCONF, 0, 0,		setip6flags },
	{ "-autoconf",	IN6_IFF_AUTOCONF, 0, 0,		resetip6flags },
	{ "tentative",	IN6_IFF_TENTATIVE, 0, 0,	setip6flags },
	{ "-tentative",	IN6_IFF_TENTATIVE, 0, 0,	resetip6flags },
	{ "deprecated",	1,		PS_PLTIME, 0,	setip6deprecated },
	{ "-deprecated", 0,		0, 0,		setip6deprecated },
	{ "pltime",	NEXTARG,	PS_PLTIME, 0,	setip6lifetime },
	{ "vltime",	NEXTARG,	PS_VLTIME, 0,	setip6lifetime },

#ifdef XNS
	    /* XNS specific */
	{ "ipdst",	NEXTARG,	PS_IPDST, 0,
	    setifipdst,	AF_NS },
#endif

#ifdef ISO
	    /* ISO specific */
	{ "snpaoffset",	NEXTARG,	PS_SNPAOFFSET, 0,
	    setsnpaoffset,	AF_ISO },
	{ "nsellength",	NEXTARG,	PS_NSELLENGTH, 0,
	    setnsellength,	AF_ISO },
#endif

	    /* Link parameters */
	{ "linktype",	NEXTARG,	PS_LINKTYPE, 0,	setlinktype },
	{ "laddr",	NEXTARG,	PS_LADDR, 0,	setladdr },
	{ "linkaddr",	NEXTARG,	PS_LADDR, 0,	setladdr },

	{ "media",	NEXTARG,	0, 0,		setmedia },

	    /* 802.11 specific */
	{ "station",	NEXTARG,	0, PS2_STATION,	setstation },
	{ "ssid",	NEXTARG,	0, PS2_SSID,	setssid },
	{ "channel",	NEXTARG,	0, PS2_CHANNEL,	setchannel },
	{ "wepkey",	NEXTARG,	0, PS2_WEPKEY,	setwepkey },
	{ "wep",	0,		0, PS2_WEPENABLE, setwepenable },
	{ "-wep",	0,		0, PS2_WEPENABLE, setwepdisable },
	{ "weptxkey",	NEXTARG,	0, PS2_WEPTXKEY, setweptxkey },
	{ "authmode",	NEXTARG,	0, PS2_AUTHMODE, setauthmode },

	    /* PIF releated keywords */
	{ "pif",	NEXTARG,	PS_ADDPIF, 0,	addpif },
	{ "-pif",	NEXTARG,	PS_RMPIF, 0,	removepif },
	{ "pifflags",	NEXTARG,	0, 0,		dopifflags } ,

	    /* VLAN related keywords */
	{ "vparent",	NEXTARG,	PS_VPARENT, 0,	setvparent },
	{ "-vparent",	0,		PS_DVPARENT|PS_VPARENT|PS_VID, 0 },
	{ "vid",	NEXTARG,	PS_VID,	 0,	setvid },

	    /* AIF related keywords */
	{ "aif",	NEXTARG,	0, PS2_ADDAIF,	addaif },
	{ "-aif",	NEXTARG,	0, PS2_RMAIF,	removeaif },
	{ "aifaddr",	NEXTARG,	0, PS2_AIFADDR,	setaifaddr },
	{ "-aifaddr",	NEXTARG,	0, PS2_RMAIFADDR, setaifaddr },
	{ "aifvid",	NEXTARG,	0, PS2_AIFVID,	setaifvid },
	{ "aifether",	NEXTARG,	0, PS2_AIFETHER, setaifether },
	{ "aifif",	NEXTARG,	0, PS2_AIFIF,	setaifif },
	{ "-aifif",	NEXTARG,	0, PS2_RMAIFIF,	setaifif },
	{ "aifflags",	NEXTARG,	0, 0,		doaifflags } ,
	{ "aifmap",	0,		0, PS2_ADDRMAP, 0 } ,

	    /* tunnel */
	{ "tunnel",	NEXTARG2,	0, PS2_TUNNEL,	NULL, 0, 0, settunnel },
	{ "deletetunnel",	0,	0, PS2_DELETETUNNEL,	0 },

	{ "ifcap",	NEXTARG,	0, 0,		doifcapflags },

/* These two must be in this order and at the end of the list */
	{ NULL,		0,		PS_ADDR, 0,	setifaddr },
	{ "destination address",
	    0,		PS_DSTADDR, 0,	setifdstaddr }
};

/* Arguments for the pifflags keyword */
struct pifflags_cmd pifflags_cmd[] = {
	/* First, the enumerated values */
	{ "first-idle",		1, PIF_MASK,		PIF_FIRST_IDLE },
	{ "next-idle",		1, PIF_MASK,		PIF_NEXT_IDLE },
	{ "first-up",		1, PIF_MASK,		PIF_FIRST_UP },
	{ "next-up",		1, PIF_MASK,		PIF_NEXT_UP },

	/* Next, the individual bits for PIF_MASK */
	{ "rescan",		1, PIF_RESCAN,		PIF_RESCAN },
	{ "-rescan",		0, PIF_RESCAN,		0 },
	{ "ignore-oactive",	1, PIF_IGNORE_OACTIVE,	PIF_IGNORE_OACTIVE },
	{ "-ignore-oactive",	0, PIF_IGNORE_OACTIVE,	0 },
	{ "reserved1",		1, PIF_RESERVED1,	PIF_RESERVED1 },
	{ "-reserved1",		0, PIF_RESERVED1,	0 },
	{ "reserved2",		1, PIF_RESERVED2,	PIF_RESERVED2 },
	{ "-reserved2",		0, PIF_RESERVED2,	0 },
	{ "reserved3",		1, PIF_RESERVED3,	PIF_RESERVED3 },
	{ "-reserved3",		0, PIF_RESERVED3,	0 },

	{ "default",		0, ~0,			0, },
	{ NULL,			0, ~0,			0, }
};

/* Arguments for the aifflags keyword */
struct aifflags_cmd aifflags_cmd[] = {
	{ "roaming",		1, AIF_ROAMING,		AIF_ROAMING },
	{ "-roaming",		0, AIF_ROAMING,		0 },
	{ "arpok",		1, AIF_ARPOK,		AIF_ARPOK },
	{ "-arpok",		0, AIF_ARPOK,		0 },
	{ "default",		0, ~0,			0, },
	{ NULL,			0, ~0,			0, }
};

/* Arguments for the ifcap keyword */
struct ifcapflags_cmd ifcapflags_cmd[] = {
	{ "rxcsum",		IFCAP_RXCSUM,		IFCAP_RXCSUM },
	{ "-rxcsum",		IFCAP_RXCSUM,		0 },
	{ "txcsum",		IFCAP_TXCSUM,		IFCAP_TXCSUM },
	{ "-txcsum",		IFCAP_TXCSUM,		0 },
	{ "netcons",		IFCAP_NETCONS,		IFCAP_NETCONS },
	{ "-netcons",		IFCAP_NETCONS,		0 },
	{ "vlanmtu",		IFCAP_VLAN_MTU,		IFCAP_VLAN_MTU },
	{ "-vlanmtu",		IFCAP_VLAN_MTU,		0 },
	{ "vlantagging",	IFCAP_VLAN_HWTAGGING,	IFCAP_VLAN_HWTAGGING },
	{ "-vlantagging",	IFCAP_VLAN_HWTAGGING,	0 },
	{ "jumbo",		IFCAP_JUMBO_MTU,	IFCAP_JUMBO_MTU },
	{ "-jumbo",		IFCAP_JUMBO_MTU,	0 },
	{ "tcpseg",		IFCAP_TCPSEG,		IFCAP_TCPSEG },
	{ "-tcpseg",		IFCAP_TCPSEG,		0 },
	{ "ipsec",		IFCAP_IPSEC,		IFCAP_IPSEC },
	{ "-ipsec",		IFCAP_IPSEC,		0 },
	{ "rxcsum6",		IFCAP_RXCSUMv6,		IFCAP_RXCSUMv6 },
	{ "-rxcsum6",		IFCAP_RXCSUMv6,		0 },
	{ "txcsum6",		IFCAP_TXCSUMv6,		IFCAP_TXCSUMv6 },
	{ "-txcsum6",		IFCAP_TXCSUMv6,		0 },
	{ "tcpseg6",		IFCAP_TCPSEGv6,		IFCAP_TCPSEGv6 },
	{ "-tcpseg6",		IFCAP_TCPSEGv6,		0 },
	{ "ipcomp",		IFCAP_IPCOMP,		IFCAP_IPCOMP },
	{ "-ipcomp",		IFCAP_IPCOMP,		0 },
	{ "cap0",		IFCAP_CAP0,		IFCAP_CAP0 },
	{ "-cap0",		IFCAP_CAP0,		0 },
	{ "cap1",		IFCAP_CAP1,		IFCAP_CAP1 },
	{ "-cap1",		IFCAP_CAP1,		0 },
	{ "cap2",		IFCAP_CAP2,		IFCAP_CAP2 },
	{ "-cap2",		IFCAP_CAP2,		0 },
	{ "cap3",		IFCAP_CAP3,		IFCAP_CAP3 },
	{ "-cap3",		IFCAP_CAP3,		0 },
	{ "all",		~0LL,			~0LL },
	{ "-all",		~0LL,			0 },
	{ "clear",		~0LL,			0 },
	{ NULL,			~0LL,			0 }
};


static struct afswitch afs[] = {
	/* First entry is default address family */
	{ "inet", AF_INET,
	    { SA(&in_addreq.ifra_addr),
		SA(&in_addreq.ifra_mask),
		SA(&in_addreq.ifra_broadaddr) },
	    in_status,	in_getaddr, in_compare,
	    SIOCDIFADDR,	SIOCAIFADDR,
	    &ridreq,
	    (struct ifaliasreq *)&in_addreq },
	{ "inet6", AF_INET6,
	    { SA(&inet6_addreq.ifra_addr),
		SA(&inet6_addreq.ifra_prefixmask),
		SA(&inet6_addreq.ifra_broadaddr) },
	    inet6_status, inet6_getaddr, inet6_compare,
	    SIOCDIFADDR_IN6, SIOCAIFADDR_IN6,
	    (struct ifreq *)&inet6_ridreq,
	    (struct ifaliasreq *)&inet6_addreq },
#ifdef XNS
	{ "ns", AF_NS,
	    { &addreq.ifra_addr,
		&addreq.ifra_mask,
		&addreq.ifra_broadaddr },
	    xns_status,	xns_getaddr, xns_compare,
	    SIOCDIFADDR,	SIOCAIFADDR,
	    &ridreq,	&addreq },
#endif
#ifdef ISO
	{ "iso", AF_ISO,
	    { SA(&iso_addreq.ifra_addr),
		SA(&iso_addreq.ifra_mask),
		SA(&iso_addreq.ifra_broadaddr) },
	    iso_status,	iso_getaddr, iso_compare,
	    SIOCDIFADDR_ISO, SIOCAIFADDR_ISO,
	    (struct ifreq *)&iso_ridreq,
	    (struct ifaliasreq *)&iso_addreq },
#endif
	{ "link",	AF_LINK,	{ NULL },	link_status },
	{ NULL },
};

static struct llswitch lls[] = {
	{ "none", IFT_NONE, },
	{ "other", IFT_OTHER, },
	{ "1822", IFT_1822, },
	{ "hdh1822", IFT_HDH1822, },
	{ "x25ddn", IFT_X25DDN, },
	{ "x25", IFT_X25, },
	{ "ether", IFT_ETHER, },
	{ "iso88023", IFT_ISO88023, },
	{ "token_bus", IFT_ISO88024, },
	{ "iso88024", IFT_ISO88024, },
	{ "token_ring", IFT_ISO88025, },
	{ "iso88025", IFT_ISO88025, },
	{ "iso88026", IFT_ISO88026, },
	{ "starlan", IFT_STARLAN, },
	{ "pronet10", IFT_P10, },
	{ "p10", IFT_P10, },
	{ "pronet80", IFT_P80, },
	{ "p80", IFT_P80, },
	{ "hy", IFT_HY, },
	{ "fddi", IFT_FDDI, },
	{ "lapb", IFT_LAPB, },
	{ "sdlc", IFT_SDLC, },
	{ "t1", IFT_T1, },
	{ "cept", IFT_CEPT, },
	{ "isdnbasic", IFT_ISDNBASIC, },
	{ "isdnprimary", IFT_ISDNPRIMARY, },
	{ "chdlc", IFT_PTPSERIAL, },
	{ "ptpserial", IFT_PTPSERIAL, },
	{ "ppp", IFT_PPP, },
	{ "loop", IFT_LOOP, },
	{ "eon", IFT_EON, },
	{ "xether", IFT_XETHER, },
	{ "nsip", IFT_NSIP, },
	{ "slip", IFT_SLIP, },
	{ "ultra", IFT_ULTRA, },
	{ "ds3", IFT_DS3, },
	{ "sip", IFT_SIP, },
	{ "frelay", IFT_FRELAY, },
	{ "fr", IFT_FRELAY, },
	{ "rs232", IFT_RS232, },
	{ "para", IFT_PARA, },
	{ "arcnet", IFT_ARCNET, },
	{ "arcnetplus", IFT_ARCNETPLUS, },
	{ "atm", IFT_ATM, },
	{ "miox25", IFT_MIOX25, },
	{ "sonet", IFT_SONET, },
	{ "x25ple", IFT_X25PLE, },
	{ "iso88022llc", IFT_ISO88022LLC, },
	{ "localtalk", IFT_LOCALTALK, },
	{ "smdsdxi", IFT_SMDSDXI, },
	{ "frelaydce", IFT_FRELAYDCE, },
	{ "v35", IFT_V35, },
	{ "hssi", IFT_HSSI, },
	{ "hippi", IFT_HIPPI, },
	{ "modem", IFT_MODEM, },
	{ "aal5", IFT_AAL5, },
	{ "sonetpath", IFT_SONETPATH, },
	{ "sonetvt", IFT_SONETVT, },
	{ "smdsicip", IFT_SMDSICIP, },
	{ "propvirtual", IFT_PROPVIRTUAL, },
	{ "propmux", IFT_PROPMUX, },
	{ "802.1Q", IFT_L2VLAN, },
	{ "802.11", IFT_IEEE80211, },
	{ 0, },
};

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct afswitch *afp;
	struct ifreq ifr;
	int aflag, ch, add_ifaddr, del_ifaddr, nsip_route, vflag;
	int flags;
	u_int64_t flags64;
	char *ifname;
	int i;

	add_ifaddr = del_ifaddr = nsip_route = 0;
	for (i = 0; i < NWEPKEYS; i++)
		wepkeylen[i] = -1;

	/* Parse flags */
	aflag = vflag = 0;
	while ((ch = getopt(argc, argv, "aLmv")) != EOF)
		switch (ch) {
		case 'a':
			aflag++;
			break;

		case 'L':
			Lflag++;
			break;

		case 'm':
			mflag++;
			break;

		case 'v':
			vflag++;
			break;

		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

	/* Find the address of AF_LINK entry */
	for (afs_link = afs; afs_link->af_name != NULL; afs_link++)
		if (afs_link->af_af == AF_LINK)
			break;
	if (afs_link->af_af != AF_LINK)
		err(1, "internal error finding AF_LINK in afs");

	/* Get current interface configuration */
	if (getifaddrs(&ifaddrs) < 0)
		err(1, "getifaddrs");

	/* Aflag means print status about all interfaces */
	if (aflag) {
		if (argc > 1)
			usage();
		if (argc == 1) {
			afp = parse_af(*argv);
			if (afp == NULL)
				usage();
		} else
			afp = NULL;

		/* Open a socket for media queries */
		if ((s = socket((afp ? afp : afs)->af_af, SOCK_DGRAM, 0)) < 0)
			err(1, "%s socket", afs->af_name);

		status(NULL, afp);
		exit(0);
	}

	/* We need at least an interface name */
	if (argc < 1)
		usage();

	/* Save interface name and make sure it is valid */
	ifname = *argv++;
	argc--;
	if (numaddrs(ifname, AF_LINK) == 0)
		errx(1, "no such interface: %s", ifname);

	/* Check for address family */
	if (argc > 0) {
		afp = parse_af(*argv);
		if (afp != NULL) {
			argv++;
			argc--;
		}
	} else
		afp = NULL;

	/* Open a socket for the specified family */
	if ((s = socket((afp ? afp : afs)->af_af, SOCK_DGRAM, 0)) < 0)
		err(1, "%s socket", (afp ? afp : afs)->af_name);

	/* No more args means a status request */
	if (argc == 0) {
		status(ifname, afp);
		exit(0);
	}

	/* initialization */
	inet6_addreq.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;
	inet6_addreq.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;

	/* Default to first address family (INET) */
	if (afp == NULL)
		afp = afs;

	/*
	 * Parse the supplied arguments.
	 */
	while (argc > 0) {
		struct cmd *p;

		/* Lookup the current arg as a command */
		for (p = cmds; p->c_name != NULL; p++)
			if (strcasecmp(*argv, p->c_name) == 0)
				break;

		/*
		 * Address and destination address are the last
		 * elements in the list.  If we did not match a
		 * command name, assume it is the address, unless we
		 * already have an address, then it is the destination
		 * address.
		 */
		if (p->c_name == NULL &&
		    ((parms_set & p->c_flag) || (parms_set2 & p->c_flag2)))
			p++;


		/* Supress duplicate and conflicting parameters */
		if (p->c_flag && (parms_set & p->c_flag)) {
			if (p->c_parameter == NEXTARG && argc > 1)
				errx(1, "conflicting or duplicate parameter: "
				    "%s %s", *argv, argv[1]);
			else
				errx(1, "conflicting or duplicate parameter: "
				    "%s", *argv);
		} else
			parms_set |= p->c_flag;

		if (p->c_flag2 && (parms_set2 & p->c_flag2)) {
			if (p->c_parameter == NEXTARG && argc > 1)
				errx(1, "conflicting or duplicate parameter: "
				    "%s %s", *argv, argv[1]);
			else
				errx(1, "conflicting or duplicate parameter: "
				    "%s", *argv);
		} else
			parms_set2 |= p->c_flag2;

		/* Verify that this option is valid for this address */
		/* family */
		if (p->c_af != AF_UNSPEC && p->c_af != afp->af_af)
			errx(1, "'%s' not valid for %s address",
			    p->c_name == NULL ? "address" : p->c_name,
			    afp->af_name);

		if (p->c_func) {
			/* If required, fetch the next argument */
			if (p->c_parameter == NEXTARG) {
				if (argc == 1)
					errx(1, "'%s' requires argument",
					    p->c_name);
				argc--, argv++;
			}
			(*p->c_func)(afp, p, *argv);
		}
		else if (p->c_func2) {
			/* If required, fetch the next argument */
			if (p->c_parameter == NEXTARG2) {
				if (argc < 3)
					errx(1, "'%s' requires argument",
					    p->c_name);
				argc--, argv++;
			}
			(*p->c_func2)(afp, p, argv[0], argv[1]);
			argc--, argv++;
		}
		argc--, argv++;
	}


	/*
	 * Figure out what we need to do based on the supplied info
	 */

	/* Set VLAN parameters */
	if ((parms_set & (PS_VPARENT|PS_VID)) &&
		(parms_set & PS_DVPARENT) == 0) {
		strncpy(vreq.vlr_name, ifname, sizeof (vreq.vlr_name));
		if (ioctl(s, SIOCSETVLAN, (caddr_t)&vreq) < 0)
			err(1, "SIOCSETVLAN");
	}

	if (parms_set & PS_IPDST) {
		/* XNS in IP encapsulation */
		nsip_route++;
	} else if (parms_set & PS_ALIAS) {
		/* For compatibility, no checking */
		if (parms_set & PS_DELETE) {
			if (parms_set & PS_ADDR)
				memcpy(&afp->af_ridreq->ifr_addr,
				    afp->af_stab[ADDR],
				    afp->af_stab[ADDR]->sa_len);
			else
				afp->af_ridreq->ifr_addr.sa_family = afp->af_af;
			del_ifaddr++;
		} else {
			add_ifaddr++;
		}
	} else if (parms_set & PS_ADD) {
		/*
		 * Add an address to an interface.  At least the
		 * address must be specified and it must not already
		 * be configured on the interface.
		 */
		if (!(parms_set & PS_ADDR))
			errx(1, "must specify address to be added");
		if (findaddr(afp, ifname, afp->af_stab[ADDR]) != NULL)
			errx(1, "specified address already configured, "
			    "use modify to change attributes",
			    ifname);
		add_ifaddr++;
	} else if (parms_set & PS_REMOVE) {
		/*
		 * Delete an address from an interface.  If the
		 * address is specified, it must exist.  Otherwise
		 * there must be only one address configured.
		 */
		if (parms_set & PS_ADDR) {
			if (findaddr(afp, ifname, afp->af_stab[ADDR]) == NULL)
				errx(1, "specified address not configured",
				    ifname);
			memcpy(&afp->af_ridreq->ifr_addr, afp->af_stab[ADDR],
			    afp->af_stab[ADDR]->sa_len);
		} else if (numaddrs(ifname, afp->af_af) > 1)
			errx(1, "must specify address to be removed");
		else
			afp->af_ridreq->ifr_addr.sa_family = afp->af_af;
		del_ifaddr++;
	} else if (parms_set & PS_MODIFY) {
		if (findaddr(afp, ifname, afp->af_stab[ADDR]) == NULL)
			errx(1, "specified address not configured",
			    ifname);
		add_ifaddr++;
	} else if (parms_set & PS_ADDRATTR) {
		/*
		 * Old style set, only allow if there are not multiple
		 * addresses.  Not supported by inet6.
		 */
		if (afp->af_af == AF_INET6)
			errx(1, "compatibility mode not supported for \"%s\", "
			    "use add or modify.",
			    afp->af_name);
		if (numaddrs(ifname, afp->af_af) > 1)
			errx(1, "multiple %s addresses configured, "
			    "use add or modify.",
			    ifname, afp->af_name);
		add_ifaddr++;
		if (parms_set & PS_ADDR) {
			del_ifaddr++;
			afp->af_ridreq->ifr_addr.sa_family = afp->af_af;
		} else
			afp->af_addreq->ifra_addr.sa_family = afp->af_af;
	}

	/*
	 * Now change the interface configuration!
	 */

	/* Set and/or reset flags */
	if (ifflags_set != 0 || ifflags_reset != 0) {
		strncpy(ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
		if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0)
			Perror("ioctl (SIOCGIFFLAGS)");

		flags = (ifr.ifr_flags & ~ifflags_reset) | ifflags_set;

		if (flags != ifr.ifr_flags) {
			ifr.ifr_flags = flags;
			if (ioctl(s, SIOCSIFFLAGS, &ifr) < 0)
				Perror("setting interface flags");
		}
	}

	/* Reset VLAN parameters */
	if (parms_set & PS_DVPARENT) {
		strncpy(vreq.vlr_name, ifname, sizeof (vreq.vlr_name));
		if (ioctl(s, SIOCSETVLAN, (caddr_t)&vreq) < 0)
			err(1, "SIOCSETVLAN");
	}

	/* Set and/or reset pif flags */
	if (pifflags_set || pifflags_reset) {
		memset(&pifreq, 0, sizeof(pifreq));
		strncpy(pifreq.pifr_name, ifname, sizeof(pifreq.pifr_name));
		if (ioctl(s, PIFIOCGFLG, &pifreq) < 0)
			Perror("ioctl (PIFIOCGFLG)");

		flags = (pifreq.pifr_flags & ~pifflags_reset) | pifflags_set;

		if (flags != pifreq.pifr_flags) {
			pifreq.pifr_flags = flags;
			if (ioctl(s, PIFIOCSFLG, &pifreq) < 0)
				Perror("setting interface pif flags");
		}
	}

	/* Set and/or reset aif flags */
	if (aifflags_set || aifflags_reset) {
		memset(&aifreq, 0, sizeof(aifreq));
		strncpy(aifreq.aifr_name, ifname, sizeof(aifreq.aifr_name));
		if (ioctl(s, AIFIOCGFLG, &aifreq) < 0)
			Perror("ioctl (AIFIOCGFLG)");

		flags = (aifreq.aifr_flags & ~aifflags_reset) | aifflags_set;

		if (flags != aifreq.aifr_flags) {
			aifreq.aifr_flags = flags;
			if (ioctl(s, AIFIOCSFLG, &aifreq) < 0)
				Perror("setting interface aif flags");
		}
	}

	/* Set and/or reset ifcap flags */
	if (ifcap_set || ifcap_reset) {
		memset(&ifcapreq, 0, sizeof(ifcapreq));
		strncpy(ifcapreq.ifcr_name, ifname, sizeof(ifcapreq.ifcr_name));
		if (ioctl(s, SIOCGIFCAP, &ifcapreq) < 0)
			Perror("ioctl (SIOCGIFCAP)");

		flags64 = (ifcapreq.ifcr_capenable & ~ifcap_reset) | ifcap_set;
		if (flags64 & ~ifcapreq.ifcr_capabilities) {
			if (ifcap_set != ~0LL)
				ifcap_warn(flags64 & ~ifcapreq.ifcr_capabilities);
			flags64 &= ifcapreq.ifcr_capabilities;
		}

		if (flags64 != ifcapreq.ifcr_capenable) {
			ifcapreq.ifcr_capenable = flags64;
			if (ioctl(s, SIOCSIFCAP, &ifcapreq) < 0)
				Perror("setting interface ifcap flags");
		}
	}

	/* Set the Metric if requested */
	if (parms_set & PS_METRIC) {
		strncpy(ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
		ifr.ifr_metric = ifmetric;
		if (ioctl(s, SIOCSIFMETRIC, &ifr) < 0)
			warn("setting metric");
	}

	/* Set the MTU if requested */
	if (parms_set & PS_MTU) {
		strncpy(ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
		ifr.ifr_mtu = ifmtu;
		if (ioctl(s, SIOCSIFMTU, &ifr) < 0)
			warn("setting mtu");
	}

#ifdef ISO
	/* Set the nsellength for all addresses */
	if (afp->af_af == AF_ISO)
		adjust_nsellength(afp);
#endif

#ifdef XNS
	/* Set XNS encapsulation routing */
	if (nsip_route) {
		struct nsip_req rq;
		int size = sizeof(rq);

		rq.rq_ns = addreq.ifra_addr;
		rq.rq_ip = addreq.ifra_dstaddr;

		if (setsockopt(s, 0, SO_NSIP_ROUTE, &rq, size) < 0)
			Perror("Encapsulation Routing");
	}
#endif

	/* Remove the old interface address */
	if (del_ifaddr) {
		/*
		 * When changing the interface address, we delete an
		 * address that is all zeros, which is shorthand for
		 * the first address on the interface.
		 * When deleting an alias, we need to specify and
		 * actual address.
		 */
		strncpy(afp->af_ridreq->ifr_name, ifname,
		    sizeof afp->af_ridreq->ifr_name);
		if (ioctl(s, afp->af_difaddr, afp->af_ridreq) < 0) {
			/*
			 * Do not log an EADDRNOTAVAIL error when
			 * doing an address change.
			 */
			if (errno != EADDRNOTAVAIL ||
			    parms_set & (PS_REMOVE|PS_DELETE))
				Perror("ioctl (SIOCDIFADDR)");
		}
	}

	/* Set new link level info */
	if (parms_set & PS_LINKTYPE) {
		struct sockaddr_dl *dl;
		int rs;

		if (getifaddrs(&ifaddrs) < 0)
			err(1, "getifaddrs");
		if ((dl = SDL(findaddr(afs_link, ifname, NULL)))
		    != NULL &&
		    dl->sdl_type != SDL(&dlreq.ifr_addr)->sdl_type)
			scrubaddrs(ifname);

		if ((rs = socket(AF_LINK, SOCK_RAW, 0)) < 0)
			Perror("socket(SOCK_RAW)");

		dl = SDL(&dlreq.ifr_addr);
		dl->sdl_family = AF_LINK;
		dl->sdl_len = sizeof(*dl) - sizeof(dl->sdl_data) +
		    dl->sdl_nlen + dl->sdl_alen + dl->sdl_slen;

		strncpy(dlreq.ifr_name, ifname, sizeof dlreq.ifr_name);
		if (ioctl(rs, SIOCSIFADDR, &dlreq) < 0)
			Perror("ioctl (SIOCSIFADDR) for linktype");
	}

	/* Deal with media changes */
	while (parms_set & (PS_MEDIAFLAG | PS_MEDIAINST | PS_LINKTYPE |
	    PS_MEDIATYPE)) {
		struct ifmediareq mreq;
		int mword;
		int lt;

		/* Snag current media options word */
		strncpy(mreq.ifm_name, ifname, sizeof (mreq.ifm_name));
		mreq.ifm_count = 0;
		if (ioctl(s, SIOCGIFMEDIA, &mreq) < 0) {
			/* Old/p2p are OK if only link type changed */
			if ((parms_set & (PS_MEDIAFLAG | PS_MEDIAINST |
			    PS_MEDIATYPE)) == 0) {
				break;
			}
			err(1, "Media operations not supported on interface");
		}
		mword = mreq.ifm_current;
		/* Apply changes */
		if ((parms_set & (PS_MEDIAINST|PS_MEDIATYPE|PS_LINKTYPE)) ||
		    ((parms_set & PS_MEDIAFLAG) &&
		    (mflags_set|mflags_clr) & ~mreq.ifm_mask)) {
			/*
			 * If we touch any non-global flags clear all the
			 * non global flags first; also do this if changing
			 * media type or link type.
			 */
			mword &= ~IFM_GMASK | mreq.ifm_mask;
		}
		if ((parms_set & PS_LINKTYPE) != 0) {
			/* Convert IFT to IFM */
			switch (cur_ltype) {
			case IFT_ETHER:
				lt = IFM_ETHER;
				break;
			case IFT_IEEE80211:
				lt = IFM_802_11;
				break;
			case IFT_ISO88025:
				lt = IFM_TOKEN;
				break;
			case IFT_FDDI:
				lt = IFM_FDDI;
				break;
			default:
				errx(1, "Unsupported link type");
			}
			/* If changed link type must set media type as well */
			if ((parms_set & PS_MEDIATYPE) == 0 &&
			    lt != (mword & IFM_NMASK))
				errx(1, "Must set media type when changing "
				    "link type");
			mword = (mword & ~IFM_NMASK) | lt;
		}
		if (parms_set & PS_MEDIAFLAG) {
			mword |= mflags_set;
			mword &= ~mflags_clr;
		}
		if (parms_set & PS_MEDIAINST)
			mword = (mword & ~IFM_IMASK) | minst << IFM_ISHIFT;
		if (parms_set & PS_MEDIATYPE) {
			switch (cur_mtype) {
			case -1:		/* UTP */
				/* Meaning up UTP depend on link type */
				switch (mword & IFM_NMASK) {
				case IFM_ETHER:
					cur_mtype = IFM_10_T;
					break;
				case IFM_TOKEN:
					cur_mtype = IFM_TOK_UTP16;
					break;
				case IFM_FDDI:
					cur_mtype = IFM_FDDI_UTP;
					break;
				}
				/* Fall through */
			default:
				mword = (mword & ~IFM_TMASK) | cur_mtype;
			}
		}

		/* Attempt to install changed media word */
		if (mword != mreq.ifm_current) {
			strncpy(ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
			ifr.ifr_media = mword;
			if (ioctl(s, SIOCSIFMEDIA, &ifr) < 0) {
				if (errno == ENXIO)
					errx(1, "%s: media settings not "
					    "valid: %s", ifname,
					    media_display_word(mword, 1));
				err(1, "Failed to set media options");
			}
		}
		break;
	}

	/* Set the new interface address (and mask and destination */
	if (add_ifaddr) {
		strncpy(afp->af_addreq->ifra_name, ifname, IFNAMSIZ);
		if (ioctl(s, afp->af_aifaddr, afp->af_addreq) < 0)
			Perror("ioctl (SIOCAIFADDR)");
		else if ((ifflags_reset & IFF_UP) != 0) {
			/*
			 * Configuring an interface address brings it
			 * up, if we have a request to bring it down
			 * we may have to repeat it now.
			 */
			strncpy(ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
			if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0)
				Perror("ioctl (SIOCGIFFLAGS)");

			flags = (ifr.ifr_flags & ~IFF_UP);

			if (flags != ifr.ifr_flags) {
				ifr.ifr_flags = flags;
				if (ioctl(s, SIOCSIFFLAGS, &ifr) < 0)
					Perror("setting interface flags");
			}
		}
	}
	if (parms_set & (PS_ADDPIF|PS_RMPIF)) {
		memset(&pifreq, 0, sizeof(pifreq));
		strncpy(pifreq.pifr_subname, ifname,
		    sizeof(pifreq.pifr_subname));
		if (parms_set & PS_RMPIF) {
			strncpy(pifreq.pifr_name, pifname_rm, 
			    sizeof(pifreq.pifr_name));
			if (ioctl(s, PIFIOCDEL, &pifreq) < 0)
				switch (errno) {
				case ENXIO:
					errx(1, "%s is not linked to %s",
					    ifname, pifname_rm);
				default:
					Perror("ioctl (PIFIOCADD)");
				}
		}
		if (parms_set & PS_ADDPIF) {
			strncpy(pifreq.pifr_name, pifname_add, 
			    sizeof(pifreq.pifr_name));
			if (ioctl(s, PIFIOCADD, &pifreq) < 0)
				switch (errno) {
				case EBUSY:
					errx(1, "%s: already linked to a pif",
					    ifname);
				case EOPNOTSUPP:
					errx(1, "%s: not supported for PIF",
					    ifname);
				default:
					Perror("ioctl (PIFIOCADD)");
				}
		}
	}
	if (parms_set2 & (PS2_ADDAIF|PS2_RMAIF)) {
		memset(&aifreq, 0, sizeof(aifreq));
		strncpy(aifreq.aifr_subname, ifname,
		    sizeof(aifreq.aifr_subname));
		if (parms_set2 & PS2_RMAIF) {
			strncpy(aifreq.aifr_name, aifname_rm, 
			    sizeof(aifreq.aifr_name));
			if (ioctl(s, AIFIOCDEL, &aifreq) < 0)
				switch (errno) {
				case ENXIO:
					errx(1, "%s is not linked to %s",
					    ifname, aifname_rm);
				default:
					Perror("ioctl (AIFIOCDEL)");
				}
		}
		if (parms_set2 & PS2_ADDAIF) {
			strncpy(aifreq.aifr_name, aifname_add, 
			    sizeof(aifreq.aifr_name));
			if (ioctl(s, AIFIOCADD, &aifreq) < 0)
				switch (errno) {
				case EBUSY:
					errx(1, "%s: already linked to a aif",
					    ifname);
				case EOPNOTSUPP:
					errx(1, "%s: not supported for AIF",
					    ifname);
				default:
					Perror("ioctl (AIFIOCADD)");
				}
		}
	}
	if (parms_set2 & (PS2_RMAIFADDR | PS2_RMAIFIF | PS2_AIFADDR |
	    PS2_AIFVID | PS2_AIFETHER | PS2_AIFIF)) {
		int cmd;
		char *errstr;

		memset(&aifreq, 0, sizeof(aifreq));
		if (parms_set2 & PS2_RMAIFIF) {
			if (parms_set2 & (PS2_AIFVID|PS2_AIFETHER|PS2_AIFIF))
				errx(1, "aifif, aifvid and aifether not "
				    "allowed with -aifif");
			cmd = AIFIOCDADDRI;
			errstr = "ioctl(AIFIOCDADDRI)";
		} else if (parms_set2 & PS2_RMAIFADDR) {
			if (parms_set2 & (PS2_AIFVID | PS2_AIFETHER))
				errx(1, "aifvid and aifether not "
				    "allowed with -aifaddr");
			cmd = AIFIOCDADDR;
			errstr = "ioctl(AIFIOCDADDR)";
		} else if (parms_set2 & PS2_AIFADDR) {
			if (parms_set2 & PS2_AIFVID) {
				aifreq.aifr_vid = aif_vid;
				cmd = AIFIOCSADDRV;
				errstr = "ioctl(AIFIOCSADDRV)";
			} else if (parms_set2 & PS2_AIFETHER) {
				bcopy(aif_ether, aifreq.aifr_ether,
				    sizeof(aifreq.aifr_ether));
				cmd = AIFIOCSADDRE;
				errstr = "ioctl(AIFIOCSADDRE)";
			} else if (parms_set2 & PS2_AIFIF) {
				strncpy(aifreq.aifr_subname, aif_subname,
				    sizeof(aifreq.aifr_subname));
				cmd = AIFIOCSADDRI;
				errstr = "ioctl(AIFIOCSADDRI)";
			} else
				errx(1, "missing aifif, aifether or aifvid");
		} else
			errx(1, "missing aifaddr, -aifaddr or -aifif");

		strncpy(aifreq.aifr_name, ifname, sizeof(aifreq.aifr_name));
		if (parms_set2 & (PS2_AIFADDR|PS2_RMAIFADDR)) {
			aifreq.aifr_addr = SA(&aifaddr);
			aifreq.aifr_addrlen = SA(&aifaddr)->sa_len;
		} else {
			aifreq.aifr_addr = NULL;
			aifreq.aifr_addrlen = 0;
		}
		if (ioctl(s, cmd, &aifreq) < 0)
			Perror(errstr);
	}
	if (parms_set & PS_LADDR)
		installladdr(ifname);

	if (parms_set2 & PS2_ADDRMAP)
		dumpaifmap(ifname);

	if (parms_set2 & PS2_STATION)
		installstation(ifname);
	if (parms_set2 & PS2_SSID)
		installssid(ifname);
	if (parms_set2 & PS2_CHANNEL)
		installchannel(ifname);

	/* First, install WEP parameters, _then_ enable WEP */
	if (parms_set2 & PS2_WEPKEY)
		installwepkey(ifname);
	if (parms_set2 & PS2_WEPTXKEY)
		installweptxkey(ifname);
	if (parms_set2 & PS2_AUTHMODE)
		installauthmode(ifname);
	if (parms_set2 & PS2_WEPENABLE)
		installwepenable(ifname);

	if (parms_set2 & PS2_TUNNEL)
		installtunnel(ifname);
	if (parms_set2 & PS2_DELETETUNNEL)
		deletetunnel(ifname);

	if (vflag) {
		/* Get and display the new interface configuration */
		if (getifaddrs(&ifaddrs) < 0)
			err(1, "getifaddrs");
		status(ifname, afp);
	}

	exit(0);
}

static void
usage()
{
	fprintf(stderr,
	    "usage: ifconfig -a [-m] [address_family]\n"
	    "       ifconfig [-m] interface [address_family]\n"
	    "       ifconfig [-mv] interface [address_family] "
	    "[address parameters] [link parameters]\n");
	exit(1);
}

static struct afswitch *
parse_af(afname)
	const char *afname;
{
	struct afswitch *afp;

	for (afp = afs; afp->af_name != NULL; afp++)
		if (strcmp(afp->af_name, afname) == 0)
			return(afp);

	return(NULL);
}


/*ARGSUSED*/
static void
setifaddr(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{
	(*afp->af_getaddr)(afp, addr, ADDR);
}


/*ARGSUSED*/
static void
setifdstaddr(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{

	if (parms_set & PS_BROADADDR)
		errx(1, "broadcast address may not be specified "
		    "when destination address is specified");
	(*afp->af_getaddr)(afp, addr, DSTADDR);
}

static void
setifnetmask(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{

	(*afp->af_getaddr)(afp, addr, MASK);
}

static void
setifprefixlen(afp, p, arg)
	struct afswitch *afp;
	struct cmd *p;
	char *arg;
{
	unsigned long plen;
	char *ep;

	plen = strtoul(arg, &ep, 10);
	if (arg == ep)
		errx(1, "invalid prefixlen");
	if (afp->af_af == AF_INET6) {
		struct sockaddr_in6 *sin6p;
		if (plen < 0 || 128 < plen)
			errx(1, "invalid prefixlen for %s", afp->af_name);
		sin6p = (struct sockaddr_in6 *)afp->af_stab[MASK];
		in6_setmask(afp, (int)plen);
	}
#if 0
	else if (afp->af_af == AF_INET) {
		struct sockaddr_in *sinp;
		if (plen < 0 || 32 < plen)
			errx(1, "invalid prefixlen for %s", afp->af_name);
		sinp = (struct sockaddr_in *)afp->af_stab[MASK];
		prefixlen(&sinp->sin_addr, (int)plen);
	}
#endif
	else
		errx(1, "prefixlen not allowed for the AF");
}

static void
setifscopeid(afp, p, arg)
	struct afswitch *afp;
	struct cmd *p;
	char *arg;
{
	u_short scopeid;
	char *ep;

	scopeid = strtoul(arg, &ep, 0) & 0xffff;
	if (arg == ep)
		errx(1, "invalid scopeid");
	if (afp->af_af == AF_INET6) {
		struct sockaddr_in6 *sin6p;
		sin6p = (struct sockaddr_in6 *)afp->af_stab[ADDR];
		if (!IN6_IS_ADDR_SITELOCAL(&sin6p->sin6_addr))
			errx(1, "scopeid allowed only for sitelocal address");
		sin6p->sin6_scope_id = scopeid;
	} else
		errx(1, "scopeid not allowed for the AF");
}

static void
setip6flags(afp, p, arg)
	struct afswitch *afp;
	struct cmd *p;
	char *arg;
{
	struct in6_aliasreq *req;

	if (afp->af_af != AF_INET6)
		errx(1, "address flags can be only set for inet6 addresses");

	req = (struct in6_aliasreq *)afp->af_addreq;
	req->ifra_flags |= p->c_parameter;
}

static void
resetip6flags(afp, p, arg)
	struct afswitch *afp;
	struct cmd *p;
	char *arg;
{
	struct in6_aliasreq *req;

	if (afp->af_af != AF_INET6)
		errx(1, "address flags can be only set for inet6 addresses");

	req = (struct in6_aliasreq *)afp->af_addreq;
	req->ifra_flags &= ~p->c_parameter;
}

static void
setip6deprecated(afp, p, arg)
	struct afswitch *afp;
	struct cmd *p;
	char *arg;
{
	if (p->c_parameter)
		setip6lifetime(afp, p, "0");
}

static void
setip6lifetime(afp, p, arg)
	struct afswitch *afp;
	struct cmd *p;
	char *arg;
{
	time_t newval, t;
	char *ep;
	struct in6_aliasreq *req;

	t = time(NULL);
	newval = (time_t)strtoul(arg, &ep, 0);
	if (arg == ep)
		errx(1, "invalid %s", p->c_name);
	if (afp->af_af != AF_INET6)
		errx(1, "%s not allowed for the AF", p->c_name);
	req = (struct in6_aliasreq *)afp->af_addreq;
	if (p->c_flag & PS_VLTIME) {
		req->ifra_lifetime.ia6t_expire = t + newval;
		req->ifra_lifetime.ia6t_vltime = newval;
	} else if (p->c_flag & PS_PLTIME) {
		req->ifra_lifetime.ia6t_preferred = t + newval;
		req->ifra_lifetime.ia6t_pltime = newval;
	}
}

static void
setifbroadaddr(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{

	if (parms_set & PS_DSTADDR)
		errx(1, "broadcast address may not be specified "
		    "when destination address is specified");
	(*afp->af_getaddr)(afp, addr, DSTADDR);
}

static void
setifipdst(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{

	/*
	 * We rely on the fact that IP and XNS use the same request
	 * structures.
	 */
	in_getaddr(afp, addr, DSTADDR);
}

static void
setlinktype(afp, p, type)
	struct afswitch *afp;
	struct cmd *p;
	char *type;
{
	struct llswitch *ll;
	struct sockaddr_dl *dl;

	for (ll = lls; ll->ll_name != NULL; ll++)
		if (strcasecmp(type, ll->ll_name) == 0)
			break;

	if (ll->ll_name == NULL)
		errx(1, "%s: invalid link type", type);

	dl = SDL(&dlreq.ifr_addr);
	dl->sdl_type = ll->ll_type;
	if (cur_ltype && cur_ltype != ll->ll_type)
		errx(1, "%s: link type in conflict with earlier args", type);
	cur_ltype = ll->ll_type;
}

/*ARGSUSED*/
static void
nonoarp(afp, p, param)
	struct afswitch *afp;
	struct cmd *p;
	char *param;
{
	static int fence = 0;

	if (fence++ == 0)
		warnx("ARP is no longer optional: %s", param);
}

/*ARGSUSED*/
static void
notrailers(afp, p, param)
	struct afswitch *afp;
	struct cmd *p;
	char *param;
{
	static int fence = 0;

	if (fence++ == 0)
		warnx("the sending of trailers is no longer supported: %s",
		    param);
}

static void
resetifflags(afp, p, unused)
	struct afswitch *afp;
	struct cmd *p;
	char *unused;
{

	if (ifflags_set & p->c_parameter)
		errx(1, "flag conflict: %s", p->c_name);
	ifflags_reset |= p->c_parameter;
}

static void
setifflags(afp, p, unused)
	struct afswitch *afp;
	struct cmd *p;
	char *unused;
{

	if (ifflags_reset & p->c_parameter)
		errx(1, "flag conflict: %s", p->c_name);
	ifflags_set |= p->c_parameter;
}

static void
setifmetric(afp, p, metric)
	struct afswitch *afp;
	struct cmd *p;
	char *metric;
{
	char *ep;

	errno = 0;
	ifmetric = strtoul(metric, &ep, 10);
	if (*metric == 0 || *ep != 0 ||
	   (ifmetric == ULONG_MAX && errno == EINVAL))
		errx(1, "invalid metric: %s", metric);
}

static void
setifmtu(afp, p, mtu)
	struct afswitch *afp;
	struct cmd *p;
	char *mtu;
{
	char *ep;

	errno = 0;
	ifmtu = strtoul(mtu, &ep, 10);
	if (*mtu == 0 || *ep != 0 ||
	   (ifmtu == ULONG_MAX && errno == EINVAL))
		errx(1, "invalid mtu: %s", mtu);
}

/* PIF support */
static void
addpif(afp, p, pif)
	struct afswitch *afp;
	struct cmd *p;
	char *pif;
{

	if (findaddr(afs_link, pif, NULL) == NULL)
		errx(1, "invalid interface name at %s");
	pifname_add = pif;
}

static void
removepif(afp, p, pif)
	struct afswitch *afp;
	struct cmd *p;
	char *pif;
{

	if (findaddr(afs_link, pif, NULL) == NULL)
		errx(1, "invalid interface name at %s");
	pifname_rm = pif;
}

static void
dopifflags(afp, p, valstr)
	struct afswitch *afp;
	struct cmd *p;
	char *valstr;
{
	struct pifflags_cmd *pf;
	u_long set;
	char *ep, *val;

	while ((val = strsep(&valstr, ",")) != NULL) {
		for (pf = pifflags_cmd; pf->pf_name; pf++)
			if (strcasecmp(val, pf->pf_name) == 0)
				break;

		if (pf->pf_name == NULL) {
			set = strtoul(val, &ep, 0);
			if (*val == 0 || *ep != 0 ||
			    (set == ULONG_MAX && errno == EINVAL))
				errx(1, "%s: invalid argument for keyword %s",
				    val, p->c_name);
		} else
			set = pf->pf_set;

		if ((pifflags_reset | pifflags_set) & (pf->pf_reset | set))
			errx(1, "pifflags conflict: %s", val);

		pifflags_reset |= pf->pf_reset;
		pifflags_set |= set;
	}
}

/* AIF support */
static void
addaif(afp, p, aif)
	struct afswitch *afp;
	struct cmd *p;
	char *aif;
{

	if (findaddr(afs_link, aif, NULL) == NULL)
		errx(1, "invalid interface name at %s");
	aifname_add = aif;
}

static void
removeaif(afp, p, aif)
	struct afswitch *afp;
	struct cmd *p;
	char *aif;
{

	if (findaddr(afs_link, aif, NULL) == NULL)
		errx(1, "invalid interface name at %s");
	aifname_rm = aif;
}

static void
doaifflags(afp, p, valstr)
	struct afswitch *afp;
	struct cmd *p;
	char *valstr;
{
	struct aifflags_cmd *af;
	u_long set;
	char *ep, *val;

	while ((val = strsep(&valstr, ",")) != NULL) {
		for (af = aifflags_cmd; af->af_name; af++)
			if (strcasecmp(val, af->af_name) == 0)
				break;

		if (af->af_name == NULL) {
			set = strtoul(val, &ep, 0);
			if (*val == 0 || *ep != 0 ||
			    (set == ULONG_MAX && errno == EINVAL))
				errx(1, "%s: invalid argument for keyword %s",
				    val, p->c_name);
		} else
			set = af->af_set;

		if ((aifflags_reset | aifflags_set) & (af->af_reset | set))
			errx(1, "aifflags conflict: %s", val);

		aifflags_reset |= af->af_reset;
		aifflags_set |= set;
	}
}

static void
dumpaifmap(ifname)
	char *ifname;
{
        int mib[5] = { CTL_NET, PF_LINK, 0, CTL_LINK_GENERIC,
            LINK_GENERIC_ADDRMAP };
        u_char *bufp, *cp, *lp;
        size_t len;
	struct aif_map *am;
	struct sockaddr_in *dst;
	struct sockaddr_dl *sdl;
                        
        mib[2] = if_nametoindex(ifname);
        if (mib[2] < 1)
                return;
                
        len = -1;
        if (sysctl(mib, 5, NULL, &len, NULL, 0) < 0) {
		perror("dumpaifmap(): first sysctl()");
		return;
	}
	printf("%-16s %-10s %s\n", "Destination", "IF", "hw addr");
	if (len == 0) {
		printf("no entries\n");
		return;
	}

	bufp = malloc(len);
	if (bufp == NULL) {
		perror("dumpaifmap(): malloc()");
		return;
	}
        if (sysctl(mib, 5, bufp, &len, NULL, 0) < 0) {
		perror("dumpaifmap(): second sysctl()");
		return;
	}
	for (am = (struct aif_map *)bufp; am; am = am->am_next) {
		dst = (struct sockaddr_in *)am->am_dst;
		sdl = (struct sockaddr_dl *)am->am_ll;
		printf("%-16s %-10s",
		    (dst ? inet_ntoa(dst->sin_addr) : "*"),
		    (sdl->sdl_nlen ? &sdl->sdl_data[0] : "*"));
		if (sdl->sdl_alen > 0) {
			for (cp = (u_char *)LLADDR(sdl),
			    lp = cp + sdl->sdl_alen;
			    cp < lp; cp++)
				printf("%c%x", cp == (u_char *)LLADDR(sdl) ?
				    ' ' : ':', *cp);
		}
		printf("\n");
        }

	free(bufp);
}

static void
setaifaddr(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{
	struct sockaddr *save_addr;

	save_addr = afp->af_stab[ADDR];
	afp->af_stab[ADDR] = (struct sockaddr *)&aifaddr;

	(*afp->af_getaddr)(afp, addr, ADDR);

	afp->af_stab[ADDR] = save_addr;
}

static void
setaifvid(afp, p, vid)
	struct afswitch *afp;
	struct cmd *p;
	char *vid;
{

	if (parms_set2 & (PS2_AIFETHER | PS2_AIFIF))
		errx(1, "aifvid not allowed with aifether or aifif");

	aif_vid = strtoul(vid, NULL, 0);
}

static void
setaifether(afp, p, ether)
	struct afswitch *afp;
	struct cmd *p;
	char *ether;
{
	struct sockaddr_dl sdl;

	memset(&sdl, 0, sizeof(sdl));
	if (parms_set2 & (PS2_AIFVID | PS2_AIFIF))
		errx(1, "aifether not allowed with aifvid or aifif");

	sdl.sdl_len = sizeof(sdl);
	link_addr(ether, &sdl);
	if (sdl.sdl_family != AF_LINK)
		errx(1, "bad aifether address %x", sdl.sdl_family);

	bcopy(LLADDR(&sdl), aif_ether, sizeof(aif_ether));
}

static void
setaifif(afp, p, ifname)
	struct afswitch *afp;
	struct cmd *p;
	char *ifname;
{

	if (parms_set2 & (PS2_AIFVID | PS2_AIFETHER))
		errx(1, "aifif not allowed with aifvid or aifether");

	strncpy(aif_subname, ifname, sizeof(aif_subname));
}

/* VLAN support */
static void
setvparent(afp, p, ifname)
	struct afswitch *afp;
	struct cmd *p;
	char *ifname;
{

	if (findaddr(afs_link, ifname, NULL) == NULL)
		errx(1, "invalid interface name");
	strncpy(vreq.vlr_parent, ifname, sizeof(vreq.vlr_parent));
}

static void 
setvid(afp, p, arg)
	struct afswitch *afp;
	struct cmd *p;
	char *arg;
{
	char *s = arg;

	vreq.vlr_id = strtoul(s, NULL, 0);
}

/* Hardware ifcap support */

static void
doifcapflags(afp, p, valstr)
	struct afswitch *afp;
	struct cmd *p;
	char *valstr;
{
	struct ifcapflags_cmd *ifc;
	u_int64_t set;
	char *ep, *val;

	while ((val = strsep(&valstr, ",")) != NULL) {
		for (ifc = ifcapflags_cmd; ifc->ifc_name; ifc++)
			if (strcasecmp(val, ifc->ifc_name) == 0)
				break;

		if (ifc->ifc_name == NULL) {
			set = strtoull(val, &ep, 0);
			if (*val == 0 || *ep != 0 ||
			    (set == UQUAD_MAX && errno == EINVAL))
				errx(1, "%s: invalid argument for keyword %s",
				    val, p->c_name);
		} else
			set = ifc->ifc_set;

		if ((ifcap_reset | ifcap_set) & (ifc->ifc_reset | set))
			errx(1, "ifcap conflict: %s", val);

		ifcap_reset |= ifc->ifc_reset;
		ifcap_set |= set;
	}
}

#define IFCAPBITS "\020\1rxcsum\2txcsum\3netcons\4vlanmtu\5vlantagging" \
	  "\6jumbo\7tcpseg\10ipsec\11rxcsum6\12txcsum6\13tcpseg6\14ipcomp" \
	  "\15cap0\16cap1\17cap2\20cap3"

static void
ifcap_warn(bits)
	u_int64_t bits;
{
	fprintf(stderr, "\tUnsupported: ");
	fprintb(stderr, "ifcap", (u_long)bits, (u_long)0, IFCAPBITS);
	fprintf(stderr, " (ignored)\n");
}
	
static void
ifcap_status(ifname)
	char *ifname;
{
	memset(&ifcapreq, 0, sizeof(ifcapreq));
	strncpy(ifcapreq.ifcr_name, ifname, sizeof(ifcapreq.ifcr_name));
	if (ioctl(s, SIOCGIFCAP, &ifcapreq) < 0)
		return;
	if (ifcapreq.ifcr_capabilities == 0 && ifcapreq.ifcr_capenable == 0)
		return;
	fprintb(stdout, "\tifcap", (u_long)ifcapreq.ifcr_capabilities,
	    (u_long)(ifcapreq.ifcr_capabilities & ~ifcapreq.ifcr_capenable),
	    IFCAPBITS);
	printf("\n");
}



#define	IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5POINTOPOINT\6NOTRAILERS\7RUNNING\10NOARP\
\11PROMISC\12ALLMULTI\13OACTIVE\14SIMPLEX\15LINK0\16LINK1\17LINK2\20MULTICAST"

/*
 * Print a value a la the %b format of the kernel's printf
 */
static void
fprintb(fp, label, v, o, bits)
	FILE *fp;
	const char *label;
	u_int v, o;
	const char *bits;
{
	int any, i;
	char c;

	any = 0;
	if (bits && *bits == 8)
		fprintf(fp, "%s=%o", label, v);
	else
		fprintf(fp, "%s=%x", label, v);
	bits++;
	if (bits) {
		fprintf(fp, "<");
		while ((i = *bits++)) {
			if (((v|o) & (1 << (i-1)))) {
				if (any)
					fprintf(fp, ",");
				any = 1;
				if (o & (1 << (i-1)))
					fprintf(fp, "-");
				for (; (c = *bits) > 32; bits++)
					fprintf(fp, "%c", c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		fprintf(fp, ">");
	}
}

/*
 * Count the number of interface addresses in the given address family
 * configured on the given interface.
 */
static int
numaddrs(ifn, af)
	const char *ifn;
	int af;
{
	struct ifaddrs *ifa;
	int num_addrs;

	num_addrs = 0;
	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next)
		if ((ifn == NULL || strcmp(ifa->ifa_name, ifn) == 0) &&
		    ifa->ifa_addr->sa_family == af)
			num_addrs++;

	return(num_addrs);
}

/*
 * Find the specified address on the given interface.
 */
static struct ifaddrs *
findaddr(afp, ifn, sa)
	struct afswitch *afp;
	const char *ifn;
	struct sockaddr *sa;
{
	struct ifaddrs *ifa;

	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next)
		if ((ifn == NULL || strcmp(ifa->ifa_name, ifn) == 0) &&
		    ifa->ifa_addr->sa_family == afp->af_af &&
		    (sa == NULL ||
			(ifa->ifa_addr->sa_len == sa->sa_len &&
			    afp->af_compare(ifa->ifa_addr, sa))))
			break;

	return(ifa);
}

/*
 * Remove all addresses from the interface.
 */
static void
scrubaddrs(ifn)
	const char *ifn;
{
	struct ifaddrs *ifa;
	struct afswitch *afp;

	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (strcmp(ifa->ifa_name, ifn) != 0 ||
		    ifa->ifa_addr->sa_family == AF_LINK)
			continue;
		
		for (afp = afs; afp->af_name != NULL; afp++)
			if (ifa->ifa_addr->sa_family == afp->af_af)
				break;

		if (afp->af_name == NULL) {
			warnx("unable to scrub family %u address",
			    ifa->ifa_addr->sa_family);
			continue;
		}

		strncpy(afp->af_ridreq->ifr_name, ifn,
		    sizeof(afp->af_ridreq->ifr_name));
		afp->af_ridreq->ifr_addr.sa_family = afp->af_af;
		if (ioctl(s, afp->af_difaddr, afp->af_ridreq) < 0 &&
		    errno != EADDRNOTAVAIL)
			Perror("scrubbing interface addresses");
	}
}

/*
 * Print the status of the interface.  If an address family was
 * specified, show it and it only; otherwise, show them all.
 */
static void
status(ifn, afp)
	const char *ifn;
	struct afswitch *afp;
{
	struct ifaddrs *ifa, *link;
	struct afswitch *p;

	link = NULL;

	/* Find the first, last and linkp entries for this interface */
	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifn != NULL && strcmp(ifa->ifa_name, ifn) != 0)
			continue;

		if (afp == NULL) {
			if (ifa->ifa_addr->sa_family == AF_LINK) {
				link_status(ifa);
				continue;
			}
			for (p = afs; p->af_name != NULL; p++)
				if (p->af_af == ifa->ifa_addr->sa_family) {
					p->af_status(ifa);
					break;
				}
		} else {
			if (ifa->ifa_addr->sa_family == AF_LINK)
				link = ifa;
			if (afp->af_af == ifa->ifa_addr->sa_family) {
				if (afp->af_af != AF_LINK && link != NULL) {
					link_status(link);
					link = NULL;
				}
				afp->af_status(ifa);
			}
		}
	}
}

static void
tunnel_status(ifa)
	struct ifaddrs *ifa;
{
	struct if_laddrreq ifl;
	struct sockaddr *sa;
	char hbuf[NI_MAXHOST];
#ifdef NI_WITHSCOPEID
	const int niflags = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
	const int niflags = NI_NUMERICHOST;
#endif

	memset(&ifl, 0, sizeof(ifl));
	strncpy(ifl.iflr_name, ifa->ifa_name, sizeof(ifl.iflr_name));
	if (ioctl(s, SIOCGLIFPHYADDR, (caddr_t)&ifl) < 0)
		return;

	sa = (struct sockaddr *)&ifl.addr;
	if (getnameinfo(sa, sa->sa_len, hbuf, sizeof(hbuf), NULL, 0, niflags))
		strcpy(hbuf, "?");
	printf("\ttunnel %s ", hbuf);
	sa = (struct sockaddr *)&ifl.dstaddr;
	if (getnameinfo(sa, sa->sa_len, hbuf, sizeof(hbuf), NULL, 0, niflags))
		strcpy(hbuf, "?");
	printf("%s\n", hbuf);
}

static void
link_status(ifa)
	struct ifaddrs *ifa;
{
	struct if_data *ifd;
	struct llswitch *ll;
	struct sockaddr_dl *sdl;
	double baudrate;
	char *multi;
	u_char *cp, *lp;

	sdl = SDL(ifa->ifa_addr);

	printf("%.*s: ", sdl->sdl_nlen, sdl->sdl_data);
	printb("flags", (u_short)ifa->ifa_flags, IFFBITS);

	printf("\n\tlink type ");

	for (ll = lls; ll->ll_name != NULL; ll++)
		if (ll->ll_type == sdl->sdl_type)
			break;

	if (ll->ll_name != NULL)
		printf("%s", ll->ll_name);
	else
		printf("%u", sdl->sdl_type);

	if (sdl->sdl_alen > 0) {
		for (cp = (u_char *)LLADDR(sdl), lp = cp + sdl->sdl_alen;
		    cp < lp; cp++)
			printf("%c%x",
			    cp == (u_char *)LLADDR(sdl) ? ' ' : ':', *cp);
	}
	if ((ifd = ifa->ifa_data) == NULL)
		warnx("no if_data");
	else {
		if (ifd->ifi_metric)
			printf(" metric %lu", ifd->ifi_metric);
		if (ifd->ifi_mtu)
			printf(" mtu %lu", ifd->ifi_mtu);
		if ((baudrate = (double)ifd->ifi_baudrate)) {
			if (baudrate >= 1000000000.0) {
				baudrate /= 1000000000.0;
				multi = "G";
			} else if (baudrate >= 1000000.0) {
				baudrate /= 1000000.0;
				multi = "M";
			} else if (baudrate >= 1000.0) {
				baudrate /= 1000.0;
				multi = "k";
			} else
				multi = "";
			printf(" speed %g%sbps", baudrate, multi);
		}
	}
	printf("\n");
	media_status_current(ifa->ifa_name);
	vlan_status(ifa->ifa_name);
	if (mflag)
		media_status_all(ifa->ifa_name);
	if (sdl->sdl_type == IFT_IEEE80211)
		ieee80211_status(ifa->ifa_name);

	pif_aif_status(ifa);

	tunnel_status(ifa);

	ifcap_status(ifa->ifa_name);
}

static void
pif_aif_status(ifa)
	struct ifaddrs *ifa;
{
	struct pifflags_cmd *pf;
	struct aifflags_cmd *af;
	u_long reset;

	memset(&pifreq, 0, sizeof(pifreq));
	strncpy(pifreq.pifr_name, "pif0", sizeof(pifreq.pifr_name));
	strncpy(pifreq.pifr_subname, ifa->ifa_name,
		    sizeof(pifreq.pifr_subname));
	if (ioctl(s, PIFIOCGPIF, &pifreq) == 0)
		printf("\tattached to %s\n", pifreq.pifr_name);
	else {
		memset(&aifreq, 0, sizeof(aifreq));
		strncpy(aifreq.aifr_name, "aif0", sizeof(aifreq.aifr_name));
		strncpy(aifreq.aifr_subname, ifa->ifa_name,
			    sizeof(aifreq.aifr_subname));
		if (ioctl(s, AIFIOCGAIF, &aifreq) == 0)
			printf("\tattached to %s\n", aifreq.aifr_name);
	}

	memset(&pifreq, 0, sizeof(pifreq));
	strncpy(pifreq.pifr_name, ifa->ifa_name, sizeof(pifreq.pifr_name));
	if (ioctl(s, PIFIOCGFLG, &pifreq) == 0) {
		printf("\tpifflags");
		for (reset = 0, pf = pifflags_cmd; pf->pf_name; pf++) {
			if (pf->pf_report && (reset & pf->pf_reset) == 0 &&
			    (pifreq.pifr_flags & pf->pf_reset) == pf->pf_set) {
				printf(reset ? ",%s" : " %s", pf->pf_name);
				reset |= pf->pf_reset;
			}
		}
		if ((pifreq.pifr_flags &= ~reset) != 0)
			printf(" otherflags=0x%x", pifreq.pifr_flags);
		printf("\n");
	}

	memset(&aifreq, 0, sizeof(aifreq));
	strncpy(aifreq.aifr_name, ifa->ifa_name, sizeof(aifreq.aifr_name));
	if (ioctl(s, AIFIOCGFLG, &aifreq) == 0) {
		printf("\taifflags");
		for (reset = 0, af = aifflags_cmd; af->af_name; af++) {
			if (af->af_report && (reset & af->af_reset) == 0 &&
			    (aifreq.aifr_flags & af->af_reset) == af->af_set) {
				printf(reset ? ",%s" : " %s", af->af_name);
				reset |= af->af_reset;
			}
		}
		if ((aifreq.aifr_flags &= ~reset) != 0)
			printf(" otherflags=0x%x", aifreq.aifr_flags);
		printf("\n");
	}
}

static void
Perror(cmd)
	const char *cmd;
{

	switch (errno) {
	case ENXIO:
		errx(1, "%s: no such interface", cmd);
		break;

	case EPERM:
		errx(1, "%s: permission denied", cmd);
		break;

	default:
		err(1, "%s", cmd);
	}
}

/* INET family support */

static void
in_getaddr(afp, addr, which)
	struct afswitch *afp;
	char *addr;
	long which;
{
	struct sockaddr_in *sinp;
	struct hostent *hp;
	struct netent *np;
	char *mp;

	sinp = SIN(afp->af_stab[which]);
	sinp->sin_len = sizeof(*sinp);
	sinp->sin_family = afp->af_af;

	/* For the primary address we recognize address/mask-length */
	if ((mp = strchr(addr, '/'))) {
		struct sockaddr_in *maskp;
		u_int32_t mask;
		u_long mlen;
		char *ep;

		if (which != ADDR)
			errx(1, "prefix length is only for the "
			    "primary address");
		*mp++ = 0;
		errno = 0;
		mlen = strtoul(mp, &ep, 10);
		if (*mp == 0 || *ep != 0 ||
		   (mlen == ULONG_MAX && errno == EINVAL) || mlen > 32)
			errx(1, "invalid mask length: %s", mp);
		if (mlen > 0) {
			mask = 0x80000000;
			while (--mlen)
				mask |= mask >> 1;
		} else
			mask = 0;
		maskp = SIN(afp->af_stab[MASK]);
		maskp->sin_len = sizeof(*sinp);
		maskp->sin_family = afp->af_af;
		maskp->sin_addr.s_addr = htonl(mask);
		parms_set |= PS_NETMASK;
	}

	if ((hp = gethostbyname(addr)) != NULL) {
		if (hp->h_addr_list[1] != NULL)
			errx(1, "'%s' resolves to multiple addresses", addr);
		memcpy(&sinp->sin_addr, hp->h_addr, hp->h_length);
	} else if ((np = getnetbyname(addr)) != NULL)
		sinp->sin_addr = inet_makeaddr(np->n_net, INADDR_ANY);
	else
		errx(1, "%s: bad value", addr);
}

#ifdef IPV4CIDR
static int
prefix_length(mask, af)
	struct sockaddr *mask;
	int af;
{
	int nbytes;
	u_int8_t *ap, c;
	int nbits = 0;

	switch (af) {
	case AF_INET:
		nbytes = mask->sa_len - 4;
		if (nbytes > sizeof(struct in_addr))
			nbytes = sizeof(struct in_addr);
		ap = (u_int8_t *)&((struct sockaddr_in *)mask)->sin_addr;
		break;
	case AF_INET6:
		nbytes = mask->sa_len - 8;
		if (nbytes > sizeof(struct in6_addr))
			nbytes = sizeof(struct in6_addr);
		ap = (u_int8_t *)&((struct sockaddr_in6 *)mask)->sin6_addr;
		break;
	default:
		return(-1);
	}
	/* Step 1: count number of full bytes */
	while (nbytes > 0 && *ap == 0xff) {
		nbits += 8;
		ap++;
		nbytes--;
	}
	if (nbytes <= 0)
		return (nbits);
	if ((c = *ap)) {
		/* Step 2: count the bits in the byte */
		while (c & 0x80) {
			nbits++;
			c <<= 1;
		}
		/* If any more are set, they aren't contiguous... */
		if (c)
			return(-1);
		--nbytes;
		ap++;
	}
	/* The rest of the bytes should all be zero */
	while (nbytes > 0) {
		if (*ap)
			return(-1);
		++ap;
		--nbytes;
	}
	return(nbits);
}
#endif

static void
in_status(ifa)
	struct ifaddrs *ifa;
{
	struct sockaddr_in *sinp;
#ifdef IPV4CIDR
	int i;
#endif /* IPV4CIDR */

	sinp = SIN(ifa->ifa_addr);
	printf("\tinet %s", inet_ntoa(sinp->sin_addr));
#ifdef IPV4CIDR
	sinp = SIN(ifa->ifa_netmask);
	if ((i = prefix_length((struct sockaddr *)sinp, AF_INET)) >= 0)
		printf("/%d", i);
#endif /* IPV4CIDR */
	if (ifa->ifa_flags & IFF_POINTOPOINT) {
		sinp = (struct sockaddr_in *)ifa->ifa_dstaddr;
		printf(" --> %s", inet_ntoa(sinp->sin_addr));
	}
#ifdef IPV4CIDR
	if (i < 0)
#endif /* IPV4CIDR */
	{
		sinp = SIN(ifa->ifa_netmask);
		printf(" netmask %s", inet_ntoa(sinp->sin_addr));
	}
	if (ifa->ifa_flags & IFF_BROADCAST &&
	    ifa->ifa_broadaddr != NULL) {
		sinp = SIN(ifa->ifa_broadaddr);
		printf(" broadcast %s", inet_ntoa(sinp->sin_addr));
	}
	printf("\n");
}

static int
in_compare(s1, s2)
	struct sockaddr *s1;
	struct sockaddr *s2;
{
	struct sockaddr_in *sin1;
	struct sockaddr_in *sin2;

	sin1 = (struct sockaddr_in *)s1;
	sin2 = (struct sockaddr_in *)s2;

	return (sin1->sin_addr.s_addr == sin2->sin_addr.s_addr);
}

#ifdef XNS
/* XNS address family support */

static void
xns_getaddr(afp, addr, which)
	struct afswitch *afp;
	char *addr;
	long which;
{
	struct sockaddr_ns *sns;

	sns = SNS(afp->af_stab[which]);
	sns->sns_family = AF_NS;
	sns->sns_len = sizeof(*sns);
	sns->sns_addr = ns_addr(addr);
	if (which == MASK)
		printf("Attempt to set XNS netmask will be ineffectual\n");
}

static void
xns_status(ifa)
	struct ifaddrs *ifa;
{
	struct sockaddr_ns *sns;

	sns = SNS(ifa->ifa_addr);
	printf("\tns %s ", ns_ntoa(sns->sns_addr));
	if (ifa->ifa_flags & IFF_POINTOPOINT) { /* by W. Nesheim@Cornell */
		sns = SNS(ifa->ifa_dstaddr);
		printf("--> %s ", ns_ntoa(sns->sns_addr));
	}
	printf("\n");
}

static int
xns_compare(s1, s2)
	struct sockaddr *s1;
	struct sockaddr *s2;
{
	struct sockaddr_ns *sns1;
	struct sockaddr_ns *sns2;

	sns1 = (struct sockaddr_ns *)s1;
	sns2 = (struct sockaddr_ns *)s2;

	return (ns_neteq(sns1->sns_addr, sns2->sns_addr));
}
#endif

/* INET6 family support */

static void
inet6_getaddr(afp, addr, which)
	struct afswitch *afp;
	char *addr;
	long which;
{
	struct addrinfo req, *ai;
	struct sockaddr_in6 *sinp;
	char *mp;
	int aierr;

	sinp = SIN6(afp->af_stab[which]);
	sinp->sin6_len = sizeof(*sinp);
	if (which != MASK)
		sinp->sin6_family = afp->af_af;

	/* For the primary address we recognize address/mask-length */
	if ((mp = strchr(addr, '/'))) {
		struct sockaddr_in6 *maskp;
		u_long mlen;
		char *ep;

		if (which != ADDR)
			errx(1, "prefix length is only for the "
			    "primary address");
		*mp++ = 0;
		errno = 0;
		mlen = strtoul(mp, &ep, 10);
		if (*mp == 0 || *ep != 0 ||
		    (mlen == ULONG_MAX && errno == EINVAL) ||
		    mlen > ((afp->af_af == AF_INET6) ? 128 : 32))
			errx(1, "invalid mask length: %s", mp);

		maskp = SIN6(afp->af_stab[MASK]);
		memset(maskp, 0, sizeof(*maskp));
		maskp->sin6_len = sizeof(*maskp);
		mp = (char *)&maskp->sin6_addr;
		while (mlen > 8) {
			*(mp++) = 0xff;
			mlen -= 8;
		}
		while (mlen--)
			*mp |= (0x80 >> mlen);

		parms_set |= PS_NETMASK;
	}

	memset(&req, 0, sizeof(struct addrinfo));
	req.ai_family = afp->af_af;

	if ((aierr = getaddrinfo(addr, NULL, &req, &ai)))
		errx(1, "getaddrinfo: %s: %s", addr, gai_strerror(aierr));

	memcpy(sinp, ai->ai_addr, ai->ai_addrlen);
	freeaddrinfo(ai);
}

static void
in6_setmask(afp, prefixlen)
	struct afswitch *afp;
	int prefixlen;
{
	struct sockaddr_in6 *mask6p;
	struct in6_addr mask;
	u_char *p;

	if (prefixlen < 0 || sizeof(mask) * 8 < prefixlen)
		errx(1, "%d: bad value", prefixlen);

	p = (u_char *)&mask;
	memset(&mask, 0, sizeof(mask));
	for ( ; 8 <= prefixlen; prefixlen -= 8)
		*p++ = 0xff;
	if (prefixlen)
		*p = (0xff00 >> prefixlen) & 0xff;
	mask6p = SIN6(afp->af_stab[MASK]);
	mask6p->sin6_addr = mask;
	mask6p->sin6_len = sizeof(*mask6p);
	parms_set |= PS_NETMASK;
}

void
in6_fillscopeid(sin6)
	struct sockaddr_in6 *sin6;
{
#if defined(KAME_SCOPEID)
	if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
		sin6->sin6_scope_id =
			ntohs(*(u_int16_t *)&sin6->sin6_addr.s6_addr[2]);
		sin6->sin6_addr.s6_addr[2] = sin6->sin6_addr.s6_addr[3] = 0;
	}
#endif
}

static void
inet6_status(ifa)
	struct ifaddrs *ifa;
{
	struct sockaddr_in6 *sin6p;
	struct in6_ifreq ifr6;
	int s6;
	u_int32_t flags6;
	struct in6_addrlifetime lifetime;
	time_t t = time(NULL);
	char hbuf[NI_MAXHOST];
#ifdef NI_WITHSCOPEID
	const int niflag = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
	const int niflag = NI_NUMERICHOST;
#endif

	strncpy(ifr6.ifr_name, ifa->ifa_name, sizeof(ifr6.ifr_name));
	if ((s6 = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		perror("ifconfig: socket");
		return;
	}
	ifr6.ifr_addr = *SIN6(ifa->ifa_addr);
	if (ioctl(s6, SIOCGIFAFLAG_IN6, &ifr6) < 0) {
		perror("ifconfig: ioctl(SIOCGIFAFLAG_IN6)");
		close(s6);
		return;
	}
	flags6 = ifr6.ifr_ifru.ifru_flags6;
	ifr6.ifr_addr = *SIN6(ifa->ifa_addr);
	memset(&lifetime, 0, sizeof(lifetime));
	if (ioctl(s6, SIOCGIFALIFETIME_IN6, &ifr6) < 0) {
		perror("ifconfig: ioctl(SIOCGIFALIFETIME_IN6)");
		close(s6);
		return;
	}
	lifetime = ifr6.ifr_ifru.ifru_lifetime;
	close(s6);

	sin6p = SIN6(ifa->ifa_addr);
	in6_fillscopeid(sin6p);
	if (getnameinfo((struct sockaddr *)sin6p, sin6p->sin6_len,
			hbuf, sizeof(hbuf), NULL, 0, niflag))
		strncpy(hbuf, "", sizeof(hbuf));	/* some message? */
	printf("\tinet6 %s ", hbuf);
	if (ifa->ifa_flags & IFF_POINTOPOINT) {
		sin6p = (struct sockaddr_in6 *)ifa->ifa_dstaddr;
		if (sin6p->sin6_family == AF_INET6) {
			in6_fillscopeid(sin6p);
			if (getnameinfo((struct sockaddr *)sin6p,
					sin6p->sin6_len, hbuf, sizeof(hbuf),
					NULL, 0, niflag)) {
				/* some message? */
				strncpy(hbuf, "", sizeof(hbuf));
			}
			printf("--> %s ", hbuf);
		}
	}
	sin6p = SIN6(ifa->ifa_netmask);
	printf("prefixlen %d ", prefixlen(&sin6p->sin6_addr,
				sizeof(sin6p->sin6_addr)));
	sin6p = SIN6(ifa->ifa_addr);
	if (sin6p->sin6_scope_id)
		printf("scopeid 0x%04x ", sin6p->sin6_scope_id);
	if ((flags6 & IN6_IFF_ANYCAST) != 0)
		printf("anycast ");
	if ((flags6 & IN6_IFF_TENTATIVE) != 0)
		printf("tentative ");
	if ((flags6 & IN6_IFF_DUPLICATED) != 0)
		printf("duplicated ");
	if ((flags6 & IN6_IFF_DETACHED) != 0)
		printf("detached ");
	/*
	 * XXX: we used to have a flag for deprecated addresses, but
	 * it was obsolete except for compatibility purposes.
	 */
	if (lifetime.ia6t_pltime == 0)
		printf("deprecated ");
	if ((flags6 & IN6_IFF_AUTOCONF) != 0)
		printf("autoconf ");
	if ((flags6 & IN6_IFF_TEMPORARY) != 0)
		printf("temporary ");
#ifdef IN6_IFF_HOME
	if ((flags6 & IN6_IFF_HOME) != 0)
		printf("home ");
#endif

	if (Lflag) {
		printf("pltime ");
		if (lifetime.ia6t_preferred) {
			printf("%s ", lifetime.ia6t_preferred < t
				? "0" : sec2str(lifetime.ia6t_preferred - t));
		} else
			printf("infty ");

		printf("vltime ");
		if (lifetime.ia6t_expire) {
			printf("%s ", lifetime.ia6t_expire < t
				? "0" : sec2str(lifetime.ia6t_expire - t));
		} else
			printf("infty ");
	}

	printf("\n");
}

static int
prefixlen(val, siz)
	void *val;
	size_t siz;
{
	u_char *name = (u_char *)val;
	int byte, bit, plen = 0;

	for (byte = 0; byte < siz; byte++, plen += 8)
		if (name[byte] != 0xff)
			break;
	if (byte == siz)
		return plen;
	for (bit = 7; bit != 0; bit--, plen++)
		if (!(name[byte] & (1 << bit)))
			break;
	for ( ; bit != 0; bit--)
		if (name[byte] & (1 << bit))
			return 0;
	byte++;
	for ( ; byte < siz; byte++)
		if (name[byte])
			return 0;
	return plen;
}

static int
inet6_compare(s1, s2)
	struct sockaddr *s1;
	struct sockaddr *s2;
{
	struct sockaddr_in6 *sin1;
	struct sockaddr_in6 *sin2;

	sin1 = (struct sockaddr_in6 *)s1;
	sin2 = (struct sockaddr_in6 *)s2;

	return(IN6_ARE_ADDR_EQUAL(&sin1->sin6_addr, &sin2->sin6_addr));
}

#ifdef ISO
/* ISO address family support */

static void
iso_getaddr(afp, addr, which)
	struct afswitch *afp;
	char *addr;
	long which;
{
	struct sockaddr_iso *siso;

	siso = SISO(afp->af_stab[which]);
	siso->siso_addr = *iso_addr(addr);
	if (which == MASK) {
		siso->siso_len = TSEL(siso) - (caddr_t)(siso);
		siso->siso_nlen = 0;
	} else {
		siso->siso_len = sizeof(*siso);
		siso->siso_family = AF_ISO;
	}
}

static void
iso_status(ifa)
	struct ifaddrs *ifa;
{
	struct sockaddr_iso *siso;

	siso = SISO(ifa->ifa_addr);
	printf("\tiso %s ", iso_ntoa(&siso->siso_addr));
	if (ifa->ifa_netmask != NULL) {
		siso = SISO(ifa->ifa_netmask);
		printf(" netmask %s ", iso_ntoa(&siso->siso_addr));
	}
	if (ifa->ifa_flags & IFF_POINTOPOINT) {
		siso = SISO(ifa->ifa_dstaddr);
		printf("--> %s ", iso_ntoa(&siso->siso_addr));
	}
	printf("\n");
}

static int
iso_compare(s1, s2)
	struct sockaddr *s1;
	struct sockaddr *s2;
{
	struct sockaddr_iso *siso1;
	struct sockaddr_iso *siso2;

	siso1 = (struct sockaddr_iso *)s1;
	siso2 = (struct sockaddr_iso *)s2;

	return (memcmp(siso1, siso2, siso2->siso_len) == 0);
}

static void
setsnpaoffset(afp, p, offset)
	struct afswitch *afp;
	struct cmd *p;
	char *offset;
{
	u_long snpaoffset;
	char *ep;

	errno = 0;
	snpaoffset = strtoul(offset, &ep, 10);
	if (*offset == 0 || *ep != 0 ||
	    (snpaoffset == ULONG_MAX && errno == EINVAL))
		errx(1, "invalid snpaoffset: %s", offset);

	iso_addreq.ifra_snpaoffset = snpaoffset;
}

static void
setnsellength(afp, p, length)
	struct afswitch *afp;
	struct cmd *p;
	char *length;
{
	char *ep;

	errno = 0;
	nsellength = strtoul(length, &ep, 10);
	if (*length == 0 || *ep != 0 ||
	    (nsellength == ULONG_MAX && errno == EINVAL))
		errx(1, "invalid nsellength: %s", length);
}

static void
fixnsel(siso)
	struct sockaddr_iso *siso;
{
	if (siso->siso_family == 0)
		return;
	siso->siso_tlen = nsellength;
}

static void
adjust_nsellength(afp)
	struct afswitch *afp;
{
	fixnsel(SISO(afp->af_stab[ADDR]));
	fixnsel(SISO(afp->af_stab[DSTADDR]));
}
#endif

/**/
/* Media support */

struct media_cmd {
	const	char *m_name;		/* Argument name */
	u_int	m_mediabits;		/* Flag to set or reset */
	u_int	m_linktype;		/* Link type */
	u_int	m_parameter;		/* TRUE if this is a flag, not a type */
};

/* Media parameters */

    /*
     * Order matters!  The first command name for a given
     * media sub-type is the one used in a status display.
     */
#define MF(key, ifm, type) { key, ifm, type, PS_MEDIAFLAG }, \
	{ "-" key, ifm, type, PS_MEDIAFLAG }
#define	MI(inst) { "inst" #inst,	inst,	0,	PS_MEDIAINST }, \
	{ "instance" #inst,	inst,	0,	PS_MEDIAINST }
#define MT(key, ifm, type) { key, ifm, type, PS_MEDIATYPE }

static struct media_cmd media_cmds[] = {
	/* Ethernet */
	MT("aui",		IFM_10_5,	IFT_ETHER),
	MT("10base5",		IFM_10_5,	IFT_ETHER),
	MT("bnc",		IFM_10_2,	IFT_ETHER),
	MT("10base2",		IFM_10_2,	IFT_ETHER),
	MT("10baseT",		IFM_10_T,	IFT_ETHER),
	MT("utp",		-1,		0),		/* sp case 1 */
	MT("100baseTX",		IFM_100_TX,	IFT_ETHER),
	MT("tx",		IFM_100_TX,	IFT_ETHER),
	MT("100baseFX",		IFM_100_FX,	IFT_ETHER),
	MT("fx",		IFM_100_FX,	IFT_ETHER),
	MT("100baseT4",		IFM_100_T4,	IFT_ETHER),
	MT("t4",		IFM_100_T4,	IFT_ETHER),
	MT("100VgAnyLan",	IFM_100_VG,	IFT_ETHER),
	MT("vg",		IFM_100_VG,	IFT_ETHER),
	MT("anylan",		IFM_100_VG,	IFT_ETHER),
	MT("100baseT2",		IFM_100_T2,	IFT_ETHER),
	MT("t2",		IFM_100_T2,	IFT_ETHER),
	MT("10baseFL",		IFM_10_FL,	IFT_ETHER),
	MT("fl",		IFM_10_FL,	IFT_ETHER),
	MT("1000baseX",		IFM_1000_X,	IFT_ETHER),
	MT("1000baseT",		IFM_1000_T,	IFT_ETHER),

	/* 802.11 */
	MT("1Mb",		IFM_802_11_1,	IFT_IEEE80211),
	MT("2Mb",		IFM_802_11_2,	IFT_IEEE80211),
	MT("5.5Mb",		IFM_802_11_5,	IFT_IEEE80211),
	MT("11Mb",		IFM_802_11_11,	IFT_IEEE80211),
	MF("ad-hoc",		IFM_802_11_IBSS,	IFT_IEEE80211),
	MF("ibss",		IFM_802_11_IBSS,	IFT_IEEE80211),
	MF("ess",		IFM_802_11_ESS,	IFT_IEEE80211),
	MF("infrastructure",	IFM_802_11_ESS,	IFT_IEEE80211),

	/* Token Ring */
	MT("utp16",		IFM_TOK_UTP16,	IFT_ISO88025),
	MT("utp4",		IFM_TOK_UTP4,	IFT_ISO88025),
	MT("stp16",		IFM_TOK_STP16,	IFT_ISO88025),
	MT("stp4",		IFM_TOK_STP4,	IFT_ISO88025),
	MF("early_token_release",	IFM_TOK_ETR,	IFT_ISO88025),
	MF("etr",		IFM_TOK_ETR,	IFT_ISO88025),
	MF("early",		IFM_TOK_ETR,	IFT_ISO88025),
	MF("source_route",	IFM_TOK_SRCRT,	IFT_ISO88025),
	MF("srt",		IFM_TOK_SRCRT,	IFT_ISO88025),
	MF("all_broadcast",	IFM_TOK_ALLR,	IFT_ISO88025),
	MF("allbc",		IFM_TOK_ALLR,	IFT_ISO88025),
	MF("mulcast_functional_address", IFM_TOK_MFA,	IFT_ISO88025),
	MF("mfa",		IFM_TOK_MFA,	IFT_ISO88025),

	/* FDDI */
	MT("multimode",		IFM_FDDI_MMF,	IFT_FDDI),
	MT("mmf",		IFM_FDDI_MMF,	IFT_FDDI),
	MT("fiber",		IFM_FDDI_MMF,	IFT_FDDI),
	MT("fibre",		IFM_FDDI_MMF,	IFT_FDDI),
	MT("singlemode",	IFM_FDDI_SMF,	IFT_FDDI),
	MT("smf",		IFM_FDDI_SMF,	IFT_FDDI),
	MT("cddi",		IFM_FDDI_UTP,	IFT_FDDI),
	MF("dual_attach",	IFM_FDDI_DA,	IFT_FDDI),
	MF("dual",		IFM_FDDI_DA,	IFT_FDDI),
	MF("da",		IFM_FDDI_DA,	IFT_FDDI),

	/* Global options */
	MF("fdx",		IFM_FDX,	0),
	MF("full_duplex",	IFM_FDX,	0),
	MF("hdx",		IFM_HDX,	0),
	MF("half_duplex",	IFM_HDX,	0),
	MF("rx_flow",		IFM_RXFLOW,	0),
	MF("tx_flow",		IFM_TXFLOW,	0),
	MF("flow",		IFM_TXFLOW|IFM_RXFLOW,	0),
	MF("flag0",		IFM_FLAG0,	0),
	MF("flag1",		IFM_FLAG1,	0),
	MF("flag2",		IFM_FLAG2,	0),
	MF("loopback",		IFM_LOOP,	0),
	MT("auto",		IFM_AUTO,	0),
	MT("automedia",		IFM_AUTO,	0),
	MT("nomedia",		IFM_NONE,	0),
	MT("disc",		IFM_NONE,	0),
	MT("manual",		IFM_MANUAL,	0),

	/* Instances */
	MI(0),		MI(1),		MI(2),		MI(3),
	MI(4),		MI(5),		MI(6),		MI(7),
	MI(8),		MI(9),		MI(10),		MI(11),
	MI(12),		MI(13),		MI(14),		MI(15),

	{ NULL }
};

/*
 * Parse media keywords
 */
static void
setmedia(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{
	struct media_cmd *mp;
	char *ap, *arg;
	int *mflags, *moflags;

	ap = addr;
	while ((arg = strsep(&ap, " ,")) != NULL) {
		/* Ignore empty fields */
		if (*arg == 0)
			continue;

		for (mp = media_cmds; mp->m_name != NULL; mp++)
			if (strcasecmp(arg, mp->m_name) == 0)
				break;

		if (mp->m_name == NULL)
			errx(1, "unknown media parameters: %s", arg);

		switch (mp->m_parameter) {
		case PS_MEDIATYPE:
		case PS_MEDIAINST:
			if (parms_set & mp->m_parameter)
				errx(1, "conflicting or duplicate media "
				    "parameter: %s",
				    arg);
			break;

		default:
			break;
		}
		switch (mp->m_parameter) {
		case PS_MEDIATYPE:
			cur_mtype = mp->m_mediabits;
			if (mp->m_linktype == 0)
				break;

			if (cur_ltype != 0 && mp->m_linktype != cur_ltype)
				errx(1, "Media %s in conflict"
				    "with some media flags",
				    mp->m_name);

			cur_ltype = mp->m_linktype;
			break;

		case PS_MEDIAFLAG:
			if (mp->m_linktype != 0) {
				if (cur_ltype != 0 &&
				    mp->m_linktype != cur_ltype)
					errx(1, "Media flag %s conflicts with"
					    " media type", mp->m_name);
				cur_ltype = mp->m_linktype;
			}

			if (*arg == '-') {
				mflags = &mflags_clr;
				moflags = &mflags_set;
			} else {
				mflags = &mflags_set;
				moflags = &mflags_clr;
			}
			if (*moflags & mp->m_mediabits)
				errx(1, "media flag conflict: %s", mp->m_name);
			*mflags |= mp->m_mediabits;
			break;

		case PS_MEDIAINST:
			minst = mp->m_mediabits;
			break;

		default:
			errx(1, "internal media table error");
			break;
		}

		parms_set |= mp->m_parameter;
	}
}

/*
 * Display current interface media status
 */
static void
media_status_current(ifname)
	char *ifname;
{
	struct ifmediareq mr;

	strncpy(mr.ifm_name, ifname, sizeof(mr.ifm_name));
	mr.ifm_count = 0;
	if (ioctl(s, SIOCGIFMEDIA, &mr) < 0 || mr.ifm_count == 0)
		return;
	printf("\tmedia %s",
	    media_display_word(mr.ifm_current, 0));
	if (mr.ifm_active != mr.ifm_current)
		printf(" (%s)",
		    media_display_word(mr.ifm_active, 0));
	if ((mr.ifm_status & IFM_AVALID) != 0) {
		printf(" status ");
		switch (mr.ifm_current & IFM_NMASK) {
		case IFM_ETHER:
			if ((mr.ifm_status & IFM_ACTIVE) != 0)
				printf("active");
			else
				printf("no-carrier");
			break;
		case IFM_802_11:
			if ((mr.ifm_status & IFM_ACTIVE) != 0)
				printf("associated");
			else
				printf("not-associated");
			break;
		case IFM_TOKEN:
		case IFM_FDDI:
			if ((mr.ifm_status & IFM_ACTIVE) != 0)
				printf("inserted");
			else
				printf("no-ring");
			break;
		}
	}
	printf("\n");
}

/* The order in which multiple link types are displayed */
static int ntypes[] = { IFM_ETHER, IFM_TOKEN, IFM_FDDI, 0 };

/*
 * Display all media possibilities on an interface
 */
static void
media_status_all(ifname)
	char *ifname;
{
	struct ifmediareq mr;
	int *ip, *lp, *np;
	int ntype, multi_linktypes;

	/* Find out how many slots we need */
	strncpy(mr.ifm_name, ifname, sizeof(mr.ifm_name));
	mr.ifm_count = 0;
	if (ioctl(s, SIOCGIFMEDIA, &mr) < 0 || mr.ifm_count == 0)
		return;
	mr.ifm_ulist = malloc(sizeof(*mr.ifm_ulist) * mr.ifm_count);
	/* Now fetch list of options */
	if (ioctl(s, SIOCGIFMEDIA, &mr) < 0) {
		warn("Error reading media options on %s", ifname);
		return;
	}

	if (mr.ifm_count == 1 &&
	    *mr.ifm_ulist == mr.ifm_current &&
	    mr.ifm_mask == 0)
		return;

	/* Does this interface support more than one link type? */
	ntype = mr.ifm_current & IFM_NMASK;
	multi_linktypes = 0;
	for (ip = mr.ifm_ulist, lp = ip + mr.ifm_count; ip < lp; ip++)
		if ((*ip & IFM_NMASK) != ntype)
			multi_linktypes++;

	if (multi_linktypes) {
		if (mr.ifm_mask & IFM_GMASK) {
			printf("\tmedia options");
			if (mr.ifm_mask & IFM_FDX)
				printf(" full_duplex");
			if (mr.ifm_mask & IFM_HDX)
				printf(" half_duplex");
			if (mr.ifm_mask & IFM_RXFLOW)
				printf(" rx_flow");
			if (mr.ifm_mask & IFM_TXFLOW)
				printf(" tx_flow");
			if (mr.ifm_mask & IFM_LOOP)
				printf(" loopback");
			if (mr.ifm_mask & IFM_FLAG0)
				printf(" flag0");
			if (mr.ifm_mask & IFM_FLAG1)
				printf(" flag1");
			if (mr.ifm_mask & IFM_FLAG2)
				printf(" flag2");
			printf("\n");
		}
		for (np = ntypes; *np != 0; np++)
			for (ip = mr.ifm_ulist, lp = ip + mr.ifm_count; ip < lp;
			    ip++) {
				if ((*ip & IFM_NMASK) != *np)
					continue;
				printf("\tmedia choice %s\n",
				    media_display_word(*ip, 1));
			}
	} else {
		if (mr.ifm_mask != 0) {
			printf("\tmedia options");
			switch (mr.ifm_current & IFM_NMASK) {
			case IFM_ETHER:
				break;
			case IFM_802_11:
				if (mr.ifm_mask & IFM_802_11_IBSS)
					printf(" ad-hoc");
				if (mr.ifm_mask & IFM_802_11_ESS)
					printf(" ess");
				break;

			case IFM_TOKEN:
				if (mr.ifm_mask & IFM_TOK_ETR)
					printf(" early_token_release");
				if (mr.ifm_mask & IFM_TOK_SRCRT)
					printf(" source_route");
				if (mr.ifm_mask & IFM_TOK_ALLR)
					printf(" all_broadcast");
				if (mr.ifm_mask & IFM_TOK_MFA)
					printf(" mulcast_functional_address");
				break;
			case IFM_FDDI:
				if (mr.ifm_mask & IFM_FDDI_DA)
					printf(" dual_attach");
				break;
			default:
				printf(" %8.8x", mr.ifm_mask);
			}
			if (mr.ifm_mask & IFM_FDX)
				printf(" full_duplex");
			if (mr.ifm_mask & IFM_HDX)
				printf(" half_duplex");
			if (mr.ifm_mask & IFM_RXFLOW)
				printf(" rx_flow");
			if (mr.ifm_mask & IFM_TXFLOW)
				printf(" tx_flow");
			if (mr.ifm_mask & IFM_LOOP)
				printf(" loopback");
			if (mr.ifm_mask & IFM_FLAG0)
				printf(" flag0");
			if (mr.ifm_mask & IFM_FLAG1)
				printf(" flag1");
			if (mr.ifm_mask & IFM_FLAG2)
				printf(" flag2");
			printf("\n");
		}
		for (ip = mr.ifm_ulist, lp = ip + mr.ifm_count; ip < lp; ip++)
			printf("\tmedia choice %s\n",
			    media_display_word(*ip, 0));
	}
}

/*
 * Display a media word in text form
 */
static char *
media_display_word(w, plink)
	int w;			/* Media word to decode */
	int plink;		/* Print link info if non-zero */
{
	struct media_cmd *mp;
	const char *desc;
	u_int ltype, ntype, stype;
	static char line[1024];
	char *lp, *ep;

	lp = line;
	ep = line + sizeof(line);

	ntype = w & IFM_NMASK;
	switch (ntype) {
	case IFM_802_11:
		ltype = IFT_IEEE80211;
		break;
	case IFM_ETHER:
		ltype = IFT_ETHER;
		break;
	case IFM_TOKEN:
		ltype = IFT_ISO88025;
		break;
	case IFM_FDDI:
		ltype = IFT_FDDI;
		break;
	default:
		(void)snprintf(lp, ep - lp, "unknown media type: %u", ntype);
		return (line);
	}
	stype = w & IFM_TMASK;

	switch (stype) {
	case IFM_AUTO:
		desc = "auto";
		break;
	case IFM_MANUAL:
		desc = "manual";
		break;
	case IFM_NONE:
		desc = "none";
		break;
	default:
		for (mp = media_cmds; mp->m_name != NULL; mp++)
			if (mp->m_parameter == PS_MEDIATYPE &&
			    mp->m_linktype == ltype && mp->m_mediabits == stype)
				break;
		desc = mp->m_name == NULL ? "unknown media" : mp->m_name;
	}
	switch (ntype) {
	case IFM_ETHER:
		if (plink)
			lp += snprintf(lp, ep - lp, "linktype ether ");
		lp += snprintf(lp, ep - lp, "%s", desc);
		break;
	case IFM_802_11:
		if (plink)
			lp += snprintf(lp, ep - lp, "linktype 802.11 ");
		if (w & IFM_802_11_IBSS)
			lp += snprintf(lp, ep - lp, "ad-hoc ");
		if (w & IFM_802_11_ESS)
			lp += snprintf(lp, ep - lp, "ess ");
		w &= ~(IFM_802_11_IBSS|IFM_802_11_ESS);
		lp += snprintf(lp, ep - lp, "%s", desc);
		break;
	case IFM_TOKEN:
		if (plink)
			lp += snprintf(lp, ep - lp, "linktype token_ring ");
		lp += snprintf(lp, ep - lp, "%s", desc);
		if (w & IFM_TOK_ETR)
			lp += snprintf(lp, ep - lp, " early_token_release");
		if (w & IFM_TOK_SRCRT)
			lp += snprintf(lp, ep - lp, " source_route");
		if (w & IFM_TOK_ALLR)
			lp += snprintf(lp, ep - lp, " all_broadcast");
		if (w & IFM_TOK_MFA)
			lp += snprintf(lp, ep - lp, 
			    " mulcast_functional_address");
		break;
	case IFM_FDDI:
		if (plink)
			lp += snprintf(lp, ep - lp, "linktype fddi ");
		lp += snprintf(lp, ep - lp, "%s", desc);
		if (w & IFM_FDDI_DA)
			lp += snprintf(lp, ep - lp, " dual_attach");
		break;
	default:
		(void)snprintf(lp, ep - lp,
		    "Undecipherable media word: %8.8x\n", w);
		return (line);
	}
	if (w & IFM_FDX)
		lp += snprintf(lp, ep - lp, " full_duplex");
	if (w & IFM_HDX)
		lp += snprintf(lp, ep - lp, " half_duplex");
	if (w & IFM_RXFLOW)
		lp += snprintf(lp, ep - lp, " rx_flow");
	if (w & IFM_TXFLOW)
		lp += snprintf(lp, ep - lp, " tx_flow");
	if (w & IFM_LOOP)
		lp += snprintf(lp, ep - lp, " loopback");
	if (w & IFM_FLAG0)
		lp += snprintf(lp, ep - lp, " flag0");
	if (w & IFM_FLAG1)
		lp += snprintf(lp, ep - lp, " flag1");
	if (w & IFM_FLAG2)
		lp += snprintf(lp, ep - lp, " flag2");
	if ((w & IFM_IMASK) != 0)
		lp += snprintf(lp, ep - lp,
		    " instance%d", (w & IFM_IMASK) >> IFM_ISHIFT);

	return (line);
}

/*
 * Display vlan status (if applicatable)
 */
static void 
vlan_status(ifname)
	char *ifname;
{

	strncpy(vreq.vlr_name, ifname, sizeof (vreq.vlr_name));
	if (ioctl(s, SIOCGETVLAN, (caddr_t)&vreq) < 0)
		return;

	if (vreq.vlr_parent[0] == 0)
		return;

	printf("\tvlan parent %s vid %u\n", vreq.vlr_parent, vreq.vlr_id);
}

char *
unknownx(x)
	int x;
{
	static char buf[32];

	sprintf(buf, "unknown-%d", x);
	return(buf);
}

/*
 * Display IEEE 802.11 status
 */
static void 
ieee80211_status(ifname)
	char *ifname;
{
	int mib[7] = { CTL_NET, PF_LINK, 0, CTL_LINK_LINKTYPE, IFT_IEEE80211,
	    IEEE_802_11_SCTL_STATION, 0 };
	int c;
	char buf[64];
	size_t len;
	char sep, *sepstr;

	mib[2] = if_nametoindex(ifname);
	if (mib[2] < 1)
		return;

	mib[5] = IEEE_802_11_SCTL_STATION;
	len = sizeof(buf);
	if (sysctl(mib, 6, buf, &len, NULL, 0) == 0)
		printf("\tstation \"%.*s\"\n", (int)len, buf);

	/*
	 * We artificially limit the list of desired SSID's to 255
	 * elements.  In practice we will probably never see more
	 * than a handful (3 for an(4) and 1 for wi(4)).
	 */
	sepstr = "\tssid ";
	mib[5] = IEEE_802_11_SCTL_SSID;
	for (mib[6] = 1; mib[6] < 256; ++mib[6]) {
		len = sizeof(buf);
		if (sysctl(mib, 7, buf, &len, NULL, 0) != 0)
			break;
		if (len)
			printf("%s\"%.*s\"\n", sepstr, (int)len, buf);
		sepstr = ",";
	}

	mib[6] = 0;
	len = sizeof(buf);
	if (sysctl(mib, 7, buf, &len, NULL, 0) == 0 && len)
		printf("\tactive ssid \"%.*s\"\n", (int)len, buf);

	mib[5] = IEEE_802_11_SCTL_CHANNEL;
	len = sizeof(c);
	if (sysctl(mib, 6, &c, &len, NULL, 0) == 0)
		printf("\tchannel %d\n", c);

	sep = '\t';

	mib[5] = IEEE_802_11_SCTL_WEPENABLE;
	len = sizeof(c);
	if (sysctl(mib, 6, &c, &len, NULL, 0) == 0) {
		printf("%cwep %s", sep, c ? "enabled" : "disabled");
		sep = ' ';
	}

	mib[5] = IEEE_802_11_SCTL_AUTHMODE;
	len = sizeof(c);
	if (sysctl(mib, 6, &c, &len, NULL, 0) == 0) {
		printf("%cauthmode %s", sep,
		    c == IEEE_802_11_AUTH_NONE ? "none" :
		    c == IEEE_802_11_AUTH_OPEN ? "open" :
		    c == IEEE_802_11_AUTH_SHARED ? "shared" :
		    c == IEEE_802_11_AUTH_ENCRYPTED ? "encrypted" :
		    unknownx(c));
		sep = ' ';
	}

	mib[5] = IEEE_802_11_SCTL_WEPTXKEY;
	len = sizeof(c);
	if (sysctl(mib, 6, &c, &len, NULL, 0) == 0) {
		printf("%cweptxkey %d\n", sep, c);
		sep = ' ';
	}
}

static void
setchannel(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{
	char *e;

	channel = strtol(addr, &e, 0);

	if (e == NULL || e == addr || *e)
		errx(1, "%s: invalid channel", addr);
}

static void
installchannel(ifname)
	char *ifname;
{
	int mib[6] = { CTL_NET, PF_LINK, 0, CTL_LINK_LINKTYPE, IFT_IEEE80211,
	    IEEE_802_11_SCTL_CHANNEL };

	mib[2] = if_nametoindex(ifname);
	if (mib[2] < 1)
		errx(1, "%s: no such interface", ifname);
	if (sysctl(mib, 6, NULL, NULL, &channel, sizeof(channel)) < 0)
		err(1, "setting channel");
}

static void
setwepenable(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{
	wepenable = 1;
}

static void
setwepdisable(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{
	wepenable = 0;
}

static void
installwepenable(ifname)
	char *ifname;
{
	int mib[6] = { CTL_NET, PF_LINK, 0, CTL_LINK_LINKTYPE, IFT_IEEE80211,
	    IEEE_802_11_SCTL_WEPENABLE };

	mib[2] = if_nametoindex(ifname);
	if (mib[2] < 1)
		errx(1, "%s: no such interface", ifname);
	if (sysctl(mib, 6, NULL, NULL, &wepenable, sizeof(wepenable)) < 0)
		err(1, "setting wepenable");
}

static void
setweptxkey(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{
	char *e;

	weptxkey = strtol(addr, &e, 0);

	if (e == NULL || e == addr || *e || 
	    weptxkey < 0 || weptxkey > NWEPKEYS)
		errx(1, "%s: invalid wep key index", addr);
}

static void
setauthmode(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{
	if (strcmp(addr, "none") == 0)
		authmode = IEEE_802_11_AUTH_NONE;
	else if (strcmp(addr, "open") == 0)
		authmode = IEEE_802_11_AUTH_OPEN;
	else if (strcmp(addr, "shared") == 0)
		authmode = IEEE_802_11_AUTH_SHARED;
	else if (strcmp(addr, "encrypted") == 0)
		authmode = IEEE_802_11_AUTH_ENCRYPTED;
	else
		errx(1, "%s: invalid authentication mode", addr);
}


static void
installauthmode(ifname)
	char *ifname;
{
	int mib[6] = { CTL_NET, PF_LINK, 0, CTL_LINK_LINKTYPE, IFT_IEEE80211,
	    IEEE_802_11_SCTL_AUTHMODE };

	mib[2] = if_nametoindex(ifname);
	if (mib[2] < 1)
		errx(1, "%s: no such interface", ifname);
	if (sysctl(mib, 6, NULL, NULL, &authmode, sizeof(authmode)) < 0)
		err(1, "setting authentication mode");
}

static void
installweptxkey(ifname)
	char *ifname;
{
	int mib[6] = { CTL_NET, PF_LINK, 0, CTL_LINK_LINKTYPE, IFT_IEEE80211,
	    IEEE_802_11_SCTL_WEPTXKEY };

	mib[2] = if_nametoindex(ifname);
	if (mib[2] < 1)
		errx(1, "%s: no such interface", ifname);
	if (sysctl(mib, 6, NULL, NULL, &weptxkey, sizeof(weptxkey)) < 0)
		err(1, "setting weptxkey");
}

static void
setwepkey(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{
	int index;
	char *a = addr;
	char *cp;
	int i;

	index = strtol(a, &cp, 0);
	if (cp && *cp == ':')
		a = cp + 1;
	else
		index = 0;
	if (index < 0 || index >= NWEPKEYS)
		errx(1, "%d: invalid wepkey index", index);

	wepkeylen[index] = 0;
	if (a[0] == '-' && a[1] == '\0')
		return;

	if (a[0] == '0' && (a[1] == 'x' || a[1] == 'X')) {
		a+= 2;
		i = 0;
		while (*a && i < sizeof(wepkey[index])) {
			if (!isxdigit(a[0]) || !isxdigit(a[1]))
				errx(1, "invalid wep key: %s", addr);
			wepkey[index][i] = 
			    (digittoint(a[0]) << 4) | digittoint(a[1]);
			a += 2;
			++i;
		}
		wepkeylen[index] = i;
		return;
	} 

	/* ASCII string */
	if (strlen(a) >= sizeof(wepkey[index])) 
		errx(1, "key too long: %s", a);
	strcpy(wepkey[index], a);
	wepkeylen[index] = strlen(a);
}

static void
installwepkey(ifname)
	char *ifname;
{
	int mib[8] = { CTL_NET, PF_LINK, 0, CTL_LINK_LINKTYPE, IFT_IEEE80211,
	    IEEE_802_11_SCTL_WEPKEY, 0 };
	int i;

	mib[2] = if_nametoindex(ifname);
	if (mib[2] < 1)
		errx(1, "%s: no such interface", ifname);
	for (i = 0; i < NWEPKEYS; i++) {
		if (wepkeylen[i] < 0)
			continue;
		mib[6] = i;
		if (sysctl(mib, 7, NULL, NULL, wepkey[i], wepkeylen[i]) < 0)
			err(1, "setting wep key %d", i);
	}
}

static void
setstation(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{
	if ((stationlen = strlen(addr)) >= sizeof(station))
		errx(1, "%s: station name too long", station);
	memcpy(station, addr, stationlen);
}

static void
installstation(ifname)
	char *ifname;
{
	int mib[6] = { CTL_NET, PF_LINK, 0, CTL_LINK_LINKTYPE, IFT_IEEE80211,
	    IEEE_802_11_SCTL_STATION };

	mib[2] = if_nametoindex(ifname);
	if (mib[2] < 1)
		errx(1, "%s: no such interface", ifname);
	if (sysctl(mib, 6, NULL, NULL, station, stationlen) < 0)
		err(1, "setting station name");
}

static void
setssid(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{
	if ((ssidlen[0] = strlen(addr)) >= sizeof(ssid))
		errx(1, "%s: SSID too long", ssid);
	strcpy(ssid[0], addr);
}

static void
installssid(ifname)
	char *ifname;
{
	int mib[7] = { CTL_NET, PF_LINK, 0, CTL_LINK_LINKTYPE, IFT_IEEE80211,
	    IEEE_802_11_SCTL_SSID, 0 };

	mib[2] = if_nametoindex(ifname);
	mib[6] = 1;
	if (mib[2] < 1)
		errx(1, "%s: no such interface", ifname);
	if (sysctl(mib, 7, NULL, NULL, ssid[0], ssidlen[0]) < 0)
		err(1, "setting SSID");
}

static void
setladdr(afp, p, addr)
	struct afswitch *afp;
	struct cmd *p;
	char *addr;
{
	char *a = addr;
	laddr.dl.sdl_alen = 0;

	while (*a && laddr.dl.sdl_alen < sizeof(laddr.dl.sdl_data)) {
		if (!isxdigit(*a))
			errx(1, "invalid link address: %s(%s)", addr, a);
		laddr.dl.sdl_data[laddr.dl.sdl_alen] = digittoint(*a);
		++a;
		if (isxdigit(*a)) {
			laddr.dl.sdl_data[laddr.dl.sdl_alen] <<= 4;
			laddr.dl.sdl_data[laddr.dl.sdl_alen] |= digittoint(*a);
			++a;
		}
		if (*a == ':' || *a == ',' || *a == '.')
			++a;
		++laddr.dl.sdl_alen;
	}
}

static void
installladdr(ifname)
	char *ifname;
{
	int mib[5] = { CTL_NET, PF_LINK, 0, CTL_LINK_GENERIC,
	    LINK_GENERIC_ADDR };

	mib[2] = if_nametoindex(ifname);
	if (mib[2] < 1)
		errx(1, "%s: no such interface", ifname);

	if (getifaddrs(&ifaddrs) < 0)
		err(1, "getifaddrs");

	laddr.dl.sdl_family = AF_LINK;
	laddr.dl.sdl_len =
	    sizeof(laddr.dl) - sizeof(laddr.dl.sdl_data) + laddr.dl.sdl_alen;

	if (sysctl(mib, 5, NULL, NULL, &laddr, laddr.dl.sdl_len) < 0)
		err(1, "setting linkaddr");

}

static void
settunnel(afp, p, src, dst)
	struct afswitch *afp;
	struct cmd *p;
	char *src;
	char *dst;
{
	struct addrinfo hints, *res;
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = afp->af_af;
	hints.ai_socktype = SOCK_DGRAM;
	error = getaddrinfo(src, "0", &hints, &res);
	if (error)
		errx(1, "getaddrinfo: %s", gai_strerror(error));
	if (res->ai_next)
		errx(1, "getaddrinfo: multiple addresses");
	if (res->ai_addrlen > sizeof(tunnelsrc))
		errx(1, "address size mismatch");
	memcpy(&tunnelsrc, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);

	error = getaddrinfo(dst, "0", &hints, &res);
	if (error)
		errx(1, "getaddrinfo: %s", gai_strerror(error));
	if (res->ai_next)
		errx(1, "getaddrinfo: multiple addresses");
	if (res->ai_addrlen > sizeof(tunneldst))
		errx(1, "address size mismatch");
	memcpy(&tunneldst, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);
}

static void
installtunnel(ifname)
	char *ifname;
{
	struct if_laddrreq ifl;

	memset(&ifl, 0, sizeof(ifl));
	strncpy(ifl.iflr_name, ifname, sizeof(ifl.iflr_name));
	if (tunnelsrc.ss_len > sizeof(ifl.addr) ||
	    tunneldst.ss_len > sizeof(ifl.dstaddr))
		errx(1, "address size mismatch");
	memcpy(&ifl.addr, &tunnelsrc, tunnelsrc.ss_len);
	memcpy(&ifl.dstaddr, &tunneldst, tunneldst.ss_len);
	if (ioctl(s, SIOCSLIFPHYADDR, (caddr_t)&ifl) < 0)
		err(1, "ioctl(SIOCSLIFPHYADDR)");
}

static void
deletetunnel(ifname)
	char *ifname;
{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCDIFPHYADDR, (caddr_t)&ifr) < 0)
		err(1, "ioctl(SIOCDIFPHYADDR)");
}

static char *
sec2str(total)
	time_t total;
{
	static char result[256];
	int days, hours, mins, secs;
	int first = 1;
	char *p = result;

	if (0) {	/*XXX*/
		days = total / 3600 / 24;
		hours = (total / 3600) % 24;
		mins = (total / 60) % 60;
		secs = total % 60;

		if (days) {
			first = 0;
			p += sprintf(p, "%dd", days);
		}
		if (!first || hours) {
			first = 0;
			p += sprintf(p, "%dh", hours);
		}
		if (!first || mins) {
			first = 0;
			p += sprintf(p, "%dm", mins);
		}
		sprintf(p, "%ds", secs);
	} else
		sprintf(p, "%lu", total);

	return(result);
}

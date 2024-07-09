/*	BSDI if.c,v 2.19 2002/01/02 23:01:09 dab Exp	*/

/*
 * Copyright (c) 1983, 1988, 1993
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
 */

#ifndef lint
static char sccsid[] = "@(#)if.c	8.2 (Berkeley) 2/21/94";
#endif /* not lint */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/if_ether.h>
#include <netinet/igmp.h>
#ifdef XNS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif
#ifdef ISO
#include <netiso/iso.h>
#include <netiso/iso_var.h>
#endif
#include <arpa/inet.h>

#include <netinet6/in6_var.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <kvm.h>
#include <kvm_stat.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "netstat.h"
#include "if_util.h"

extern	kvm_t *kvmd;

static	char *etherprint __P((u_char *, char string[18]));
static	void if_print_compat __P((struct if_msghdr *, struct sockaddr_dl *,
	    char *));
static	void if_print_compat_addr __P((struct if_msghdr *, struct sockaddr_dl *,
	    char *, struct ifa_msghdr *, struct rt_addrinfo *));
static	void if_print_sub __P((struct if_msghdr *, struct sockaddr_dl *,
	    char *));
static	void if_print_sub_addr __P((struct if_msghdr *, struct sockaddr_dl *,
	    char *, struct ifa_msghdr *, struct rt_addrinfo *));
static	void if_print_multi_addr __P((struct if_msghdr *, struct sockaddr_dl *,
	    char *, struct ifa_msghdr *, struct rt_addrinfo *));
static	void if_print_multi_ifnet __P((struct if_msghdr *, struct sockaddr_dl *,
	    char *));
static	void if_stats_long __P((struct if_msghdr *, struct sockaddr_dl *, 
	    char *));
static	void if_stats_short __P((struct if_msghdr *, struct sockaddr_dl *, 
	    char *));
static	void if_stats_short_addr __P((struct if_msghdr *, struct sockaddr_dl *,
	    char *, struct ifa_msghdr *, struct rt_addrinfo *));

void
if_stats(void)
{
	if (vflag)
		if_loop(kvmd, interface, 0, if_stats_long, NULL);
	else {
		printf(
		    "%-4.4s %-5.5s %-25.25s %9.9s %5.5s %9.9s %5.5s",
		    "Name", "Index", "Address",
		    bflag ? "Ibytes" : "Ipkts", "Ierrs",
		    bflag ? "Obytes" : "Opkts", "Oerrs");
		printf(" %5s %s", "Coll", "Drop\n");

		if_loop(kvmd, interface, 0, if_stats_short, 
		    if_stats_short_addr);
	}
}

static void
if_stats_long(struct if_msghdr *ifm, struct sockaddr_dl *sdl, char *name)
{
	struct if_data *ifdp;
	double baudrate;
	char *multi;

	ifdp = &ifm->ifm_data;

#define	p(f, m) if (f || vflag > 1) \
    printf(m, (u_quad_t)f, PLURAL(f))
#define	p_5(f, m) if (f || vflag > 1) \
    printf(m, (u_quad_t)f)
#define	py(f, m) if (f || vflag > 1) \
    printf(m, (u_quad_t)f, PLURALIES(f))

	if (vflag < 2 &&
	    (ifdp->ifi_ipackets == 0 && ifdp->ifi_ierrors == 0 &&
	    ifdp->ifi_opackets == 0 && ifdp->ifi_oerrors == 0 &&
	    ifdp->ifi_collisions == 0 &&
	    ifdp->ifi_ibytes == 0 && ifdp->ifi_obytes == 0 &&
	    ifdp->ifi_imcasts == 0 && ifdp->ifi_omcasts == 0 &&
	    ifdp->ifi_iqdrops == 0 &&
	    ifdp->ifi_oqdrops == 0 &&
	    ifdp->ifi_noproto == 0))
		return;

	printf("%.*s:\n", sdl->sdl_nlen, sdl->sdl_data);
	p_5(ifdp->ifi_mtu, "\t%qu maximum transmission unit [MTU]\n");
	p_5(ifdp->ifi_metric, "\t%qu routing metric (external only)\n");
	if ((baudrate = (double)ifdp->ifi_baudrate) || vflag > 1) {
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
		printf("\t%g%s linespeed (in bits per second)\n", baudrate, multi);
	}
	p((u_quad_t)ifdp->ifi_ipackets,
	    "\t%qu packet%s rcvd on intf\n");
	p((u_quad_t)ifdp->ifi_ierrors,
	    "\t%qu input err%s on intf\n");
	p((u_quad_t)ifdp->ifi_opackets, 
	    "\t%qu packet%s sent on intf\n");
	p((u_quad_t)ifdp->ifi_oerrors,
	    "\t%qu output err%s on intf\n");
	p((u_quad_t)ifdp->ifi_collisions,
	    "\t%qu collision%s on csma intfs\n");
	p((u_quad_t)ifdp->ifi_ibytes,
	    "\t%qu total number of octet%s rcvd\n");
	p((u_quad_t)ifdp->ifi_obytes,
	    "\t%qu total number of octet%s sent\n");
	p((u_quad_t)ifdp->ifi_imcasts,
	    "\t%qu packet%s rcvd via mcast\n");
	p((u_quad_t)ifdp->ifi_omcasts,
	    "\t%qu packet%s sent via mcast\n");
	p((u_quad_t)ifdp->ifi_iqdrops,
	    "\t%qu packet%s dropped on input, this intf\n");
	p((u_quad_t)ifdp->ifi_oqdrops,
	    "\t%qu packet%s dropped on output, this intf\n");
	p((u_quad_t)ifdp->ifi_noproto,
	    "\t%qu packet%s destined for unsupported proto\n");
}

static void
if_stats_short(struct if_msghdr *ifm, struct sockaddr_dl *sdl, char *name)
{
	struct if_data *ifdp;
	int m;
	char *cp, *lp, ether[18];

	ifdp = &ifm->ifm_data;

	printf("%-7s %2u ", name, ifm->ifm_index);

	switch (sdl->sdl_type) {
	case IFT_L2VLAN:
	case IFT_ETHER:
	case IFT_FDDI:
	case IFT_ISO88025:
	case IFT_IEEE80211:
		printf("%*.*s ", PARG(-25, 25), 
		    etherprint((u_char *)LLADDR(sdl), ether));
		break;

	default:
		m = 0;
		cp = LLADDR(sdl);
		lp = cp + sdl->sdl_alen;
		if (cp >= lp) {
			printf("%*.*s ", PARG(-25, 25), filler);
			break;
		}
		do {
			m += printf("%x%c", *cp & 0xff,
			    m > 0 ? '.' : ' ');
		} while (++cp < lp);
		printf("%*s", 18 - m, "");
		break;
	}

	printf("%9qu %5qu %9qu %5qu %5qu %4qu\n",
	    bflag ? ifdp->ifi_ibytes : ifdp->ifi_ipackets,
	    ifdp->ifi_ierrors,
	    bflag ? ifdp->ifi_obytes : ifdp->ifi_opackets,
	    ifdp->ifi_oerrors, ifdp->ifi_collisions,
	    ifdp->ifi_oqdrops);
}

static void
if_stats_short_addr(struct if_msghdr *ifm, struct sockaddr_dl *sdl, char *name,
    struct ifa_msghdr *ifam, struct rt_addrinfo *rti)
{
	struct in_addr inaddr;
	int m;
	char *cp, *lp;

	printf("%-7s %2u ", name, ifm->ifm_index);

	switch (ifaddr(rti)->sa_family) {
	case AF_UNSPEC:
		printf("%*.*s ", PARG(-25, 25), filler);
		break;
	case AF_INET:
#define	SA2SINADDR(sa)	((struct sockaddr_in *)(sa))->sin_addr
#define	INNONE(a, s)	(a).s_addr == INADDR_ANY ? "none" : (s)
		inaddr = ifm->ifm_flags & IFF_POINTOPOINT ?
		    SA2SINADDR(dstaddr(rti)) : SA2SINADDR(ifaddr(rti));
		printf("%*.*s ", PARG(-25, 25),
		    INNONE(inaddr, routename(inaddr.s_addr)));
		break;
	case AF_INET6:
		if (ifm->ifm_flags & IFF_POINTOPOINT)
			p_sockaddr(dstaddr(rti), NULL, 0, 25);
		else
			p_sockaddr(ifaddr(rti), NULL, 0, 25);
		break;
#ifdef XNS
	case AF_NS:
		printf("%*.*s ", PARG(-25, 25), ns_phost(ifaddr(rti)));
		break;
#endif /* XNS */
	default:
		m = printf("(%d)", ifaddr(rti)->sa_family);
		for (cp = ifaddr(rti)->sa_data, lp = cp + ifaddr(rti)->sa_len;
		     cp < lp; cp++) 
			m += printf("%x%c", *cp & 0xff,
			    cp > ifaddr(rti)->sa_data ? '.' : ' ');
		printf("%*s", 18 - m, "");
		break;
	}

	printf("%9qu %5.5s %9qu %5.5s %5.5s\n",
	    bflag ? ifam->ifam_data.ifai_ibytes : 
	    ifam->ifam_data.ifai_ipackets,
	    filler,
	    bflag ? ifam->ifam_data.ifai_obytes : 
	    ifam->ifam_data.ifai_opackets,
	    filler, filler);
}

void
if_print_multi(void)
{

	printf("%-4.4s %-5.5s %-25.25s %-23.23s %-5.5s %-5.5s %-4.4s\n",
	    "Name", "Index", "Address", "Group", "State", "Timer",
	    "Refs");

	if_loop(kvmd, interface, fflag, if_print_multi_ifnet,
	    if_print_multi_addr);
}

static void
if_print_multi_ifnet(struct if_msghdr *ifm, struct sockaddr_dl *sdl, char *name)
{
	struct ether_multi *ep0, *ep;
	size_t len;
	char ether[18], multi[18];

	if (!(ifm->ifm_flags & IFF_MULTICAST))
		return;

	switch (sdl->sdl_type) {
	case IFT_L2VLAN:
	case IFT_ETHER:
	case IFT_FDDI:
	case IFT_ISO88025:
	case IFT_IEEE80211:
		if ((ep0 = kvm_ifmulti(kvmd, sdl, PF_LINK, &len)) == NULL)
			break;
		for (ep = len ? ep0 : NULL; ep != NULL; ep = ep->enm_next) {
			printf(
			    "%-7s %2u %*.*s %s",
			    name, ifm->ifm_index, PARG(-25, 25), 
			    etherprint((u_char *)LLADDR(sdl), ether),
			    etherprint(ep->enm_addrlo, multi));
			if (memcmp(&ep->enm_addrlo, &ep->enm_addrhi, 6) != 0)
				printf("-%s\n",
				    etherprint(ep->enm_addrhi, multi));
			else
				printf("\n");
		}
		free(ep0);
		break;

	default:
		break;
	}
}

static void
if_print_multi_addr(struct if_msghdr *ifm, struct sockaddr_dl *sdl, char *name,
    struct ifa_msghdr *ifam, struct rt_addrinfo *rti)
{
	struct in_multiaddr *ima, *ima0;
	struct in_addr *addr;
	size_t len;
	static int skip_index;
	char *aname, *state, sbuf[14];

	if (!(ifm->ifm_flags & IFF_MULTICAST))
		return;

	switch (ifaddr(rti)->sa_family) {
	case AF_INET:
		if (skip_index && skip_index == ifm->ifm_index)
			break;
		if ((ima0 = kvm_ifmulti(kvmd, sdl, PF_INET, &len)) == NULL)
			return;
		if (len == 0) {
			free(ima0);
			return;
		}
		if (ifm->ifm_flags & IFF_POINTOPOINT)
			addr = &SA2SINADDR(dstaddr(rti));
		else
			addr = &SA2SINADDR(ifaddr(rti));
		aname = strdup(routename(addr->s_addr));
		for (ima = ima0; ima != NULL; ima = ima->ima_next) {
			switch (ima->ima_state) {
			case IGMP_DELAYING_MEMBER:
				state = "Delay";
				break;
			case IGMP_IDLE_MEMBER:
				state = "Idle";
				break;
			case IGMP_LAZY_MEMBER:
				state = "Lazy";
				break;
			case IGMP_SLEEPING_MEMBER:
				state = "Sleep";
				break;
			case IGMP_AWAKENING_MEMBER:
				state = "Awake";
				break;
			default:
				(void)snprintf(sbuf, sizeof(sbuf), "%5u",
				    ima->ima_state);
				state = sbuf;
				break;
			}
			printf("%-7s %2u %*.*s %*.*s %5s %5u %4u\n",
			    name, ifm->ifm_index, PARG(-25, 25), aname,
			    PARG(-23, 23), routename(ima->ima_group.s_addr),
			    state, ima->ima_timer,
			    ima->ima_refcount);
		}
		skip_index = ifm->ifm_index;
		free(aname);
		free(ima0);
		break;

	default:
		break;
	}

}

void
if_print(void)
{
	/* Check for compatibility mode */
	if (Oflag) {
		printf("%-5.5s %-5.5s %-17.17s %-17.17s "
		    "%9.9s %5.5s %9.9s %5.5s %5s %5s\n",
		    "Name", "MTU", "Network", "Address", 
		    bflag ? "Ibytes" : "Ipkts", "Ierrs",
		    bflag ? "Obytes" : "Opkts", "Oerrs",
		    "Coll", "Drop");

		if_loop(kvmd, interface, fflag, if_print_compat, 
		    if_print_compat_addr);

		return;
	}

	printf("%-4.4s %-5.5s %5.5s %5.5s %4.4s %-25.25s %s\n",
	    "Name", "Index", "MTU", "Speed", "Mtrc", "Address", "Network");

	if_loop(kvmd, interface, fflag, if_print_sub, if_print_sub_addr);
}

static void
if_print_sub(struct if_msghdr *ifm, struct sockaddr_dl *sdl, char *name)
{
	struct if_data *ifdp;
	struct if_entry *ife;
	double baudrate;
	int m;
	char *cp, *lp, speed[20], ether[18];

	ifdp = &ifm->ifm_data;

	baudrate = (double)ifdp->ifi_baudrate;
	if (baudrate >= 1000000000.0)
		m = sprintf(speed, "%5gG", baudrate / 1000000000.0);
	else if (baudrate >= 1000000.0)
		m = sprintf(speed, "%5gM", baudrate / 1000000.0);
	else if (baudrate >= 1000.0)
		m = sprintf(speed, "%5gk", baudrate / 1000.0);
	else if (baudrate)
		m = sprintf(speed, "%6g", baudrate);
	else
		m = sprintf(speed, "%6.6s", filler);
	printf("%-7s %2u %5lu %6s %3ld ", name, ifm->ifm_index, ifdp->ifi_mtu,
	    speed, ifdp->ifi_metric);

	switch (sdl->sdl_type) {
	case IFT_L2VLAN:
	case IFT_ETHER:
	case IFT_FDDI:
	case IFT_ISO88025:
	case IFT_IEEE80211:
		m = printf("%s", etherprint((u_char *)LLADDR(sdl), ether));
		break;
	default:
		m = 0;
		cp = LLADDR(sdl);
		lp = cp + sdl->sdl_alen;
		if (cp >= lp) {
			m = printf("%s", filler);
			break;
		}
		do {
			m += printf("%x%c", *cp & 0xff,
			    m > 0 ? '.' : ' ');
		} while (++cp < lp);
		break;
	}
	if (ifm->ifm_pif != 0 &&
	    (ife = if_byindex(ifm, ifm->ifm_pif)) != NULL)
		printf(" %*s->%-*s", 25 - m, "", ife->ife_addr->sdl_nlen,
		    ife->ife_addr->sdl_data);
	putchar('\n');
}

static void
if_print_sub_addr(struct if_msghdr *ifm, struct sockaddr_dl *sdl, char *name,
    struct ifa_msghdr *ifam, struct rt_addrinfo *rti)
{
	struct if_data *ifdp;
	struct in_addr inaddr;
	char *cp, *lp;
	
	ifdp = &ifm->ifm_data;

	printf("%-7s %2u %5.5s %6.6s %3d ", name, ifm->ifm_index, filler,
	    filler, ifam->ifam_data.ifai_metric);

	switch (ifaddr(rti)->sa_family) {
	case AF_UNSPEC:
		printf("%-25.25s %s", filler, filler);
		break;
	case AF_INET:
		inaddr = SA2SINADDR(ifaddr(rti));
		printf("%*.*s ", PARG(-25, 25),
		    INNONE(inaddr, routename(inaddr.s_addr)));
		if (ifm->ifm_flags & IFF_POINTOPOINT) {
			inaddr = SA2SINADDR(dstaddr(rti));
			printf("%s",
			    INNONE(inaddr, routename(inaddr.s_addr)));
		} else if (netmask(rti) == NULL) {
			inaddr = SA2SINADDR(ifaddr(rti));
			printf("%s",
			    INNONE(inaddr, routename(inaddr.s_addr)));
		} else
			printf("%s",
			    netname(SA2SINADDR(ifaddr(rti)).s_addr & 
				SA2SINADDR(netmask(rti)).s_addr,
				ntohl(SA2SINADDR(netmask(rti)).s_addr)));
		break;
	case AF_INET6:
		p_sockaddr(ifaddr(rti), NULL, 0, 25);
		if (ifm->ifm_flags & IFF_POINTOPOINT)
			p_sockaddr(dstaddr(rti), NULL, 0, -1);
		else
			p_sockaddr(ifaddr(rti), netmask(rti), 0, -1);
		break;
#ifdef XNS
	case AF_NS: {
		struct sockaddr_ns *sns = (struct sockaddr_ns *)ifaddr(rti);
		u_long net;
		char netnum[8];

		printf("%*.*s ", PARG(-25, 25), 
		    ns_phost((struct sockaddr *)sns));
		*(union ns_net *) &net = sns->sns_addr.x_net;
		(void)sprintf(netnum, "%lXH", (u_long)ntohl(net));
		printf("ns:%s", netnum);
		break;
	}
#endif /* XNS */
	default:
		(void)printf("(%d)", ifaddr(rti)->sa_family);
		for (cp = ifaddr(rti)->sa_data, 
			 lp = cp + ifaddr(rti)->sa_len; cp < lp; cp++)
			(void)printf("%x%c", *cp & 0xff,
			    cp > ifaddr(rti)->sa_data ? '.' : ' ');
		break;
	}
	putchar('\n');
}

static void
if_print_compat(struct if_msghdr *ifm, struct sockaddr_dl *sdl, char *name)
{
	struct if_data *ifdp;
	struct if_entry *ife;
	int m;
	char *cp, *lp, ether[18];

	ifdp = &ifm->ifm_data;

	printf("%-5s %-5lu ", name, ifdp->ifi_mtu);

	m = printf("<link%d>", ifm->ifm_index);
	if (ifm->ifm_pif != 0 &&
	    (ife = if_byindex(ifm, ifm->ifm_pif)) != NULL) {
		m += printf("->%-*s", ife->ife_addr->sdl_nlen,
		    ife->ife_addr->sdl_data);
	}
	printf("%*s", 18 - m, "");

	switch (sdl->sdl_type) {
	case IFT_L2VLAN:
	case IFT_ETHER:
	case IFT_FDDI:
	case IFT_ISO88025:
	case IFT_IEEE80211:
		printf("%*.*s ", PARG(-17, 17), 
		    etherprint((u_char *)LLADDR(sdl), ether));
		break;
	default:
		m = 0;
		cp = (char *)LLADDR(sdl);
		for (cp = LLADDR(sdl), lp = cp + sdl->sdl_alen; cp < lp; cp++)
			m += printf("%x%c", *cp & 0xff, m > 0 ? '.' : ' ');
		printf("%*s", 18 - m, "");
		break;
	}

	printf("%9qu %5qu %9qu %5qu %5qu %3qu\n",
	    bflag ? ifdp->ifi_ibytes : ifdp->ifi_ipackets,
	    ifdp->ifi_ierrors,
	    bflag ? ifdp->ifi_obytes : ifdp->ifi_opackets,
	    ifdp->ifi_oerrors, ifdp->ifi_collisions,
	    ifdp->ifi_oqdrops);

	if (vflag) {
		struct ether_multi *ep0, *ep;
		size_t len;
		char multi[18];
		
		switch (ifdp->ifi_type) {
		case IFT_L2VLAN:
		case IFT_ETHER:
		case IFT_FDDI:
		case IFT_ISO88025:
		case IFT_IEEE80211:
			break;

		default:
			return;
		}

		if ((ep0 = kvm_ifmulti(kvmd, sdl, PF_LINK, &len)) == NULL)
			return;
		for (ep = len ? ep0 : NULL; ep != NULL; ep = ep->enm_next) {
			printf("%29s %s", "", 
			    etherprint(ep->enm_addrlo, multi));
			if (memcmp(&ep->enm_addrlo, &ep->enm_addrhi, 6) != 0)
				printf(" to %s",
				    etherprint(ep->enm_addrhi, multi));
			printf("\n");
		}
		free(ep0);
	}
}

static void
if_print_compat_addr(struct if_msghdr *ifm, struct sockaddr_dl *sdl, char *name,
    struct ifa_msghdr *ifam, struct rt_addrinfo *rti)
{
	struct if_data *ifdp;
	struct in_addr inaddr;
	int m;
	char *cp, *lp;

	ifdp = &ifm->ifm_data;

	printf("%-5s %-5lu ", name, ifdp->ifi_mtu);

	switch (ifaddr(rti)->sa_family) {
	case AF_UNSPEC:
		printf("%-17.17s %-17.17s ", filler, filler);
		break;
	case AF_INET:
		if (ifm->ifm_flags & IFF_POINTOPOINT) {
			inaddr = SA2SINADDR(dstaddr(rti));
			printf("%*.*s ", PARG(-17, 17),
			    INNONE(inaddr, routename(inaddr.s_addr)));
		} else if (netmask(rti) == NULL) {
			inaddr = SA2SINADDR(ifaddr(rti));
			printf("%*.*s ", PARG(-17, 17),
			    INNONE(inaddr, routename(inaddr.s_addr)));
		} else
			printf("%*.*s ", PARG(-17, 17),
			    netname(SA2SINADDR(ifaddr(rti)).s_addr & 
				SA2SINADDR(netmask(rti)).s_addr,
				ntohl(SA2SINADDR(netmask(rti)).s_addr)));
		inaddr = SA2SINADDR(ifaddr(rti));
		printf("%*.*s ", PARG(-17, 17),
		    INNONE(inaddr, routename(inaddr.s_addr)));
		break;
	case AF_INET6:
		if (ifm->ifm_flags & IFF_POINTOPOINT)
			p_sockaddr(dstaddr(rti), NULL, 0, 17);
		else
			p_sockaddr(ifaddr(rti), netmask(rti), 0, 17);
		p_sockaddr(ifaddr(rti), NULL, 0, 17);
		break;
#ifdef XNS
	case AF_NS: {
		struct sockaddr_ns *sns =
		    (struct sockaddr_ns *)ifaddr(rti);
		u_long net;
		char netnum[8];

		*(union ns_net *) &net = sns->sns_addr.x_net;
		(void)sprintf(netnum, "%lXH", (u_long)ntohl(net));
		printf("ns:%-8s ", netnum);
		printf("%-17s ", ns_phost((struct sockaddr *)sns));
		break;
	}
#endif /* XNS */
	default:
		m = printf("(%d)", ifaddr(rti)->sa_family);
		for (cp = ifaddr(rti)->sa_data, 
			 lp = cp + ifaddr(rti)->sa_len; cp < lp; cp++)
			m += printf("%x%c", *cp & 0xff,
			    cp > ifaddr(rti)->sa_data ? '.' : ' ');
		printf("%*s", 36 - m, "");
		break;
	}
	printf("%9qu %5qu %9qu %5qu %5qu %3d\n",
	    bflag ? ifdp->ifi_ibytes : ifdp->ifi_ipackets,
	    ifdp->ifi_ierrors,
	    bflag ? ifdp->ifi_obytes : ifdp->ifi_opackets,
	    ifdp->ifi_oerrors, ifdp->ifi_collisions, 0);

	if (vflag) {
		struct in_multiaddr *ima, *ima0;
		size_t len;

		switch (ifaddr(rti)->sa_family) {
		case AF_INET:
			if ((ima0 = kvm_ifmulti(kvmd, sdl, PF_INET,
			    &len)) == NULL)
				return;
			for (ima = len ? ima0 : NULL; ima != NULL; 
			     ima = ima->ima_next)
				printf("%-29s %*.*s\n", "", PARG(-19, 19),
				    routename(ima->ima_group.s_addr));
			free(ima0);
			break;

		default:
			break;
		}
	}
}

/**/

struct iftot {
	u_quad_t	ift_ip;			/* input packets (or bytes) */
	u_quad_t	ift_ie;			/* input errors */
	u_quad_t	ift_op;			/* output packets (or bytes) */
	u_quad_t	ift_oe;			/* output errors */
	u_quad_t	ift_co;			/* collisions */
	u_quad_t	ift_dr;			/* drops */
};
static struct iftot stats_total, stats_total_current;
static struct iftot stats_if, stats_if_current;
static char if_current_name[IFNAMSIZ+2];	/* Name in parens */
static int if_current_index, if_current_pref;
static int if_count;
static int screen_lines, screen_line;

static int signalled;			/* set if alarm goes off "early" */

static void sideways_pass1 __P((void));
static void sideways_pass2 __P((void));
static void sideways_print_header __P((void));
static void sideways_print_line __P((int));
static void sideways_signal __P((int));

/*
 * Print a running summary of interface statistics.
 * Repeat display every interval seconds, showing statistics
 * collected over that interval.  Assumes that interval is non-zero.
 * First line printed at top of screen is always cumulative.
 */
void
if_print_interval(int intrvl)
{
	struct sigaction sigact;
	struct itimerval itv;
	struct winsize win;
	sigset_t sigset;

	/* find an interface and collect initial stats */
	sideways_pass1();

	if (if_current_index == 0) {
		if (interface == NULL)
			errx(1, "no interface found");
		else
			errx(1, "no such interface: %s", interface);
	}

	/* Print initial line */
	sideways_print_header();
	sideways_print_line(1);
	fflush(stdout);

	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = sideways_signal;
	sigact.sa_flags = 0;
	(void)sigemptyset(&sigact.sa_mask);
	(void)sigaddset(&sigact.sa_mask, SIGINFO);
	(void)sigaddset(&sigact.sa_mask, SIGALRM);

	if (sigaction(SIGALRM, &sigact, NULL))
		err(1, "initialing SIGINT handler");

	if (sigaction(SIGINFO, &sigact, NULL))
		err(1, "initialing SIGINT handler");

	if (sigaction(SIGINT, &sigact, NULL))
		err(1, "initialing SIGINT handler");

	screen_lines = 24;
	if (isatty(STDOUT_FILENO) &&
	    ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == 0) {
		if (win.ws_row != 0)
			screen_lines = win.ws_row;
		if (sigaction(SIGWINCH, &sigact, NULL))
			err(1, "initialing SIGWINCH handler");
	}

	memset(&itv, 0, sizeof(itv));
	itv.it_value.tv_sec = itv.it_interval.tv_sec = intrvl;
	if (setitimer(ITIMER_REAL, &itv, NULL))
		err(1, "setting timer");

	(void)sigemptyset(&sigset);
	do {
		sigsuspend(&sigset);
	} while (signalled == 0);
}

static void
sideways_print_line(int delta)
{
#define	DELTA(new, old, field, delta) new.field - (delta ? old.field : 0)
	printf("%9qu %5qu %9qu %5qu %5qu",
	    DELTA(stats_if_current, stats_if, ift_ip, delta),
	    DELTA(stats_if_current, stats_if, ift_ie, delta),
	    DELTA(stats_if_current, stats_if, ift_op, delta),
	    DELTA(stats_if_current, stats_if, ift_ie, delta),
	    DELTA(stats_if_current, stats_if, ift_co, delta));
	if (vflag)
		printf(" %5qu",
		    DELTA(stats_if_current, stats_if, ift_dr, delta));
	stats_if = stats_if_current;
	
	if (if_count > 1) {
		printf(" %9qu %5qu %9qu %5qu %5qu",
		    DELTA(stats_total_current, stats_total, ift_ip, delta),
		    DELTA(stats_total_current, stats_total, ift_ie, delta),
		    DELTA(stats_total_current, stats_total, ift_op, delta),
		    DELTA(stats_total_current, stats_total, ift_ie, delta),
		    DELTA(stats_total_current, stats_total, ift_co, delta));
		if (vflag)
			printf(" %5qu",
			    DELTA(stats_total_current, stats_total, ift_dr, 
				delta));
		stats_total = stats_total_current;
	}
	printf("\n");
}

static void
sideways_print_header(void)
{
	printf("   input    %-9.9s output       ", if_current_name);
	if (if_count > 1) {
		if (vflag)
			printf("      ");
		printf("      input   (Total)    output");
	}
	printf("\n%9.9s %5.5s %9.9s %5.5s %5.5s ",
	    bflag ? "bytes" : "packets", "errs", 
	    bflag ? "bytes" : "packets", "errs", "colls");
	if (vflag)
		printf("%5.5s ", "drops");
	if (if_count > 1) {
		printf("%9.9s %5.5s %9.9s %5.5s %5.5s",
		    bflag ? "bytes" : "packets", "errs",
		    bflag ? "bytes" : "packets", "errs", "colls");
		if (vflag)
			printf(" %5.5s", "drops");
	}
	printf("\n");
	screen_line = 3;
}

static void
sideways_pass2(void)
{
	struct if_msghdr *ifm, *ifm0, *ep;
	struct if_data *idp;
	size_t len;

	if ((ifm0 = kvm_iflist(kvmd, 0, AF_MAX, &len)) == NULL)
		err(1, "kvm_iflist");
	ep = (struct if_msghdr *)((u_char *)ifm0 + len);

	if (ifm0->ifm_version != RTM_VERSION)
		errx(1, "kvm_iflist: version mismatch");

	len = interface == NULL ? 0 : strlen(interface);

	for (ifm = ifm0; ifm < ep; 
	     ifm = (struct if_msghdr *)((u_char *)ifm + ifm->ifm_msglen)) {
		if (ifm->ifm_type != RTM_IFINFO)
			continue;

		idp = (struct if_data *)&ifm->ifm_data;

		stats_total_current.ift_ip += bflag ? idp->ifi_ibytes :
		    idp->ifi_ipackets;
		stats_total_current.ift_ie += idp->ifi_ierrors;
		stats_total_current.ift_op += bflag ? idp->ifi_obytes :
		    idp->ifi_opackets;
		stats_total_current.ift_oe += idp->ifi_oerrors;
		stats_total_current.ift_co += idp->ifi_collisions;
		stats_total_current.ift_dr = idp->ifi_oqdrops;

		if (ifm->ifm_index != if_current_index)
			continue;

		stats_if_current.ift_ip = bflag ? idp->ifi_ibytes :
		    idp->ifi_ipackets;
		stats_if_current.ift_ie = idp->ifi_ierrors;
		stats_if_current.ift_op = bflag ? idp->ifi_obytes :
		    idp->ifi_opackets;
		stats_if_current.ift_oe = idp->ifi_oerrors;
		stats_if_current.ift_co = idp->ifi_collisions;
		stats_if_current.ift_dr = idp->ifi_oqdrops;
	}

	free(ifm0);
}

static void
sideways_pass1(void)
{
	struct if_msghdr *ifm, *ifm0, *ep;
	struct if_data *idp;
	struct rt_addrinfo info;
	struct sockaddr_dl *sdl;
	size_t len;
	int pref;

	if ((ifm0 = kvm_iflist(kvmd, 0, AF_MAX, &len)) == NULL)
		err(1, "kvm_iflist");
	ep = (struct if_msghdr *)((u_char *)ifm0 + len);

	if (ifm0->ifm_version != RTM_VERSION)
		errx(1, "kvm_iflist: version mismatch");

	len = interface == NULL ? 0 : strlen(interface);

	for (ifm = ifm0; ifm < ep; 
	     ifm = (struct if_msghdr *)((u_char *)ifm + ifm->ifm_msglen)) {
		if (ifm->ifm_type != RTM_IFINFO)
			continue;
		if_xaddrs(ifm + 1, ifm->ifm_addrs, &info);
		sdl = (struct sockaddr_dl *)info.rti_info[RTAX_IFP];

		if_count++;

		idp = (struct if_data *)&ifm->ifm_data;

		stats_total_current.ift_ip += bflag ? idp->ifi_ibytes :
		    idp->ifi_ipackets;
		stats_total_current.ift_ie += idp->ifi_ierrors;
		stats_total_current.ift_op += bflag ? idp->ifi_obytes :
		    idp->ifi_opackets;
		stats_total_current.ift_oe += idp->ifi_oerrors;
		stats_total_current.ift_co += idp->ifi_collisions;
		stats_total_current.ift_dr = idp->ifi_oqdrops;

		if (len != 0) {
			/* 
			 * An interface was specified, continue if we have
			 * already found it.
			 */
			if (if_current_index != 0)
				continue;
			/* Continue if this isn't the one */
			if (len != sdl->sdl_nlen ||
			    strncmp(interface, sdl->sdl_data, sdl->sdl_nlen) != 0)
				continue;
		} else {
			/* 
			 * We prefer a normal interface over a loopback
			 * interface and a loopback interface over a down
			 * interface. 
			 */
			pref = ifm->ifm_flags & IFF_UP ?
			    ifm->ifm_flags & IFF_LOOPBACK ? 2 : 3 : 1;
			if (pref <= if_current_pref)
				continue;
			if_current_pref = pref;
		}

		if_current_index = ifm->ifm_index;
		(void)snprintf(if_current_name, sizeof(if_current_name), "(%.*s%s)",
		    sdl->sdl_nlen, sdl->sdl_data, 
		    ifm->ifm_flags & IFF_UP ? "" : "*");

		stats_if_current.ift_ip = bflag ? idp->ifi_ibytes :
		    idp->ifi_ipackets;
		stats_if_current.ift_ie = idp->ifi_ierrors;
		stats_if_current.ift_op = bflag ? idp->ifi_obytes :
		    idp->ifi_opackets;
		stats_if_current.ift_oe = idp->ifi_oerrors;
		stats_if_current.ift_co = idp->ifi_collisions;
		stats_if_current.ift_dr = idp->ifi_oqdrops;
	}

	free(ifm0);
}

static void
sideways_signal(int sig)
{
	struct winsize win;

	switch (sig) {
	case SIGALRM:
		memset(&stats_total_current, 0, sizeof(stats_total_current));
		sideways_pass2();
		if (++screen_line >= screen_lines) {
			sideways_print_header();
			sideways_print_line(0);
			screen_line++;
		}
		sideways_print_line(1);
		fflush(stdout);
		break;

	case SIGINT:
		signalled = 1;
		break;

	case SIGINFO:
		sideways_print_header();
		sideways_print_line(0);
		screen_line++;
		fflush(stdout);
		break;

	case SIGWINCH:
	    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == 0 &&
		win.ws_row != 0)
		    screen_lines = win.ws_row;
	    break;
	}
}

/*
 * Return a printable string representation of an Ethernet address.
 */
static char *
etherprint(u_char *enaddr, char string[18])
{

	(void)sprintf(string, "%02x:%02x:%02x:%02x:%02x:%02x",
	    enaddr[0], enaddr[1], enaddr[2], enaddr[3], enaddr[4], enaddr[5]);

	return (string);
}

/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI kvm_iflist.c,v 2.4 2001/11/14 17:40:30 dab Exp
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <net/route.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_fddi.h>
#include <net/if_802_11.h>
#include <net/if_token.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/if_ether.h>
#include <netinet6/in6_var.h>

#include <db.h>
#include <errno.h>
#include <kvm.h>
#include <stdlib.h>
#include <string.h>

#include "kvm_private.h"

#define	SALIGN	(sizeof(long) - 1)
#define	SA_RLEN(sa)	((sa)->sa_len ? (((sa)->sa_len + SALIGN) & ~SALIGN) : (SALIGN + 1))

#define	BUFUNIT	10240

static size_t
_kvm_addrs(void *vp, struct rt_addrinfo *info)
{
	struct sockaddr *sa;
	size_t dlen, len;
	int i;
	u_char *cp;

	cp = vp;
	info->rti_addrs = len = 0;
	for (i = 0; i < RTAX_MAX; i++) {
		if ((sa = info->rti_info[i]) == NULL)
			continue;
		info->rti_addrs |= (1 << i);
		dlen = SA_RLEN(sa);
		if (cp) {
			memcpy(cp, sa, dlen);
			cp += dlen;
		}
		len += dlen;
	}

	return (len);
}

#define	ADJSA(SA, O, N) \
    ((struct sockaddr *)((u_char *)(SA) - (u_char *)(O) + (u_char *)(N)))

struct if_msghdr *
kvm_iflist(kvm_t *kd, int ifindex, int af, size_t *rlen)
{
	struct if_msghdr *ifm;
	struct ifa_msghdr *ifam;
	union {
		struct ifaddr ifa;
		struct in_ifaddr in;
		struct in6_ifaddr in6;
	} ifaddr;
	struct ifnet ifnet, *ifp;
	struct ifaddr *ifap;
	struct rt_addrinfo info;
	u_long addr, ifnetaddr;
	size_t needed;
	int buflen, blen, len;
	int mib[6];
	u_char *buf, *bp;

	if (ISALIVE(kd)) {
		mib[0] = CTL_NET;
		mib[1] = PF_ROUTE;
		mib[2] = NET_ROUTE_TABLE;	/* protocol */
		mib[3] = af;			/* wildcard address family */
		mib[4] = NET_RT_IFLIST;
		mib[5] = ifindex;		/* index */
		if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) {
			_kvm_syserr(kd, kd->program, "kvm_iflist");
			return (NULL);
		}
		if ((buf = _kvm_malloc(kd, needed)) == NULL)
			return (NULL);
		if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
			_kvm_syserr(kd, kd->program, "kvm_iflist");
			free(buf);
			return (NULL);
		}
		*rlen = needed;
		return ((struct if_msghdr *)buf);
	}

	/* Simulate the sysctl */
	if ((addr = _kvm_symbol(kd, "_ifnet")) == 0 ||
	    kvm_read(kd, addr, &ifnetaddr, sizeof(ifnetaddr)) != 
	    sizeof (ifnetaddr))
		return (NULL);

	/* Get a chunk of memory */
	buflen = BUFUNIT;
	if ((buf = _kvm_malloc(kd, buflen)) == NULL)
		return (NULL);
	bp = buf;

	/* Scan all the ifnet structures */
	for (ifp = (struct ifnet *)ifnetaddr; ifp != NULL; 
	     ifp = ifp->if_next) {
		/* Read ifnet */
		if (kvm_read(kd, (u_long)ifp, &ifnet, sizeof(ifnet))
		    != sizeof(ifnet)) {
			free(buf);
			return (NULL);
		}
		ifp = &ifnet;

		if (ifindex != 0 && ifp->if_index != ifindex)
			continue;

		/* Read the AF_LINK ifaddr */
		if (kvm_read(kd, (u_long)ifp->if_addrlist, &ifaddr,
		    sizeof(ifaddr)) != sizeof(ifaddr)) {
			free(buf);
			return (NULL);
		}
		ifap = &ifaddr.ifa;

		memset(&info, 0, sizeof(info));
		info.rti_info[RTAX_IFP] = 
		    ADJSA(ifap->ifa_addr, ifp->if_addrlist, ifap);

		/* Not what we were expecting! */
		if (info.rti_info[RTAX_IFP]->sa_family != AF_LINK)
			continue;

		/* Fill in the if_msghdr and addresses */
		ifm = (struct if_msghdr *)bp;
		memset(ifm, 0, sizeof(*ifm));
		bp = (u_char *)(ifm + 1);
		len = ALIGN(_kvm_addrs(NULL, &info));
		if (bp + len > buf + buflen) {
			blen = bp - buf;
			if ((buf = realloc(buf, buflen += BUFUNIT)) == NULL) {
				_kvm_err(kd, kd->program, strerror(errno));
				return (NULL);
			}
			bp = buf + blen;
		}
		bp += len;
		ifm->ifm_msglen = bp - (u_char *)ifm;
		ifm->ifm_version = RTM_VERSION;
		ifm->ifm_type = RTM_IFINFO;
		ifm->ifm_addrs = info.rti_addrs;
		ifm->ifm_flags = ifp->if_flags;
		ifm->ifm_index = ifp->if_index;
		ifm->ifm_data = ifp->if_data;
		(void)_kvm_addrs(ifm + 1, &info);

		/* If there is a PIF we need it's index */
		if (ifp->if_pif) {
			struct ifnet ifnet_pif;

			if (kvm_read(kd, (u_long)ifp, &ifnet_pif, 
			    sizeof(ifnet_pif)) != sizeof(ifnet_pif)) {
				free(buf);
				return (NULL);
			}
			ifm->ifm_pif = ifnet_pif.if_index;
		}

		/* Scan all the ifaddr structures */
		while ((ifap = ifap->ifa_next) != NULL) {
			if (kvm_read(kd, (u_long)ifap, &ifaddr, sizeof(ifaddr))
			    != sizeof(ifaddr)) {
				free(buf);
				return (NULL);
			}

			/* Adjust the addresses */
			memset(&info, 0, sizeof(info));
			info.rti_info[RTAX_IFA] = 
			    ADJSA(ifaddr.ifa.ifa_addr, ifap, &ifaddr);
			info.rti_info[RTAX_NETMASK] =
			    ADJSA(ifaddr.ifa.ifa_netmask, ifap, &ifaddr);
			info.rti_info[RTAX_BRD] = 
			    ADJSA(ifaddr.ifa.ifa_dstaddr, ifap, &ifaddr);

			ifap = &ifaddr.ifa;
			if (af != 0 && af != info.rti_info[RTAX_IFA]->sa_family)
				continue;

			/* Fill in the ifa_msghdr */
			ifam = (struct ifa_msghdr *)bp;
			memset(ifam, 0, sizeof(*ifam));
			bp = (u_char *)(ifam + 1);
			len = ALIGN(_kvm_addrs(NULL, &info));
			if (bp + len > buf + buflen) {
				blen = bp - buf;
				if ((buf = realloc(buf, buflen += BUFUNIT))
				    == NULL) {
					_kvm_err(kd, kd->program, strerror(errno));
					return (NULL);
				}
				bp = buf + blen;
			}
			bp += len;
			ifam->ifam_msglen = bp - (u_char *)ifam;
			ifam->ifam_version = RTM_VERSION;
			ifam->ifam_type = RTM_NEWADDR;
			ifam->ifam_addrs = info.rti_addrs;
			ifam->ifam_flags = ifap->ifa_flags;
			ifam->ifam_index = ifp->if_index;
			ifam->ifam_data = ifap->ifa_data;
			(void)_kvm_addrs(ifam + 1, &info);
		}
	}

	*rlen = bp - buf;
	return (struct if_msghdr *)buf;
}

void *
kvm_ifmulti(kvm_t *kd, struct sockaddr_dl *sdl, int pf, size_t *rlen)
{
	struct arpcom ac;
	union {
		struct ifaddr ifa;
		struct in_ifaddr in;
		struct in6_ifaddr in6;
	} ifaddr;
	struct ifaddr *ifap, **ifnet_addrs;
	struct ether_multi *enm0, *enm, *emp;
	struct in_multiaddr *inma0, *inma;
	struct in_multi *ia, *inm0, *inm, **inm1;
	struct sockaddr *ap;
	u_long addr;
	size_t needed;
	int mib[10], *mp, if_index, i;
	void *buf;

	if (ISALIVE(kd)) {
		mp = mib;
		*mp++ = CTL_NET;
		*mp++ = PF_LINK;
		*mp++ = sdl->sdl_index;
		switch (pf) {
#ifdef	CTL_LINK_LINKTYPE
		case PF_LINK:
			*mp++ = CTL_LINK_LINKTYPE;
			*mp++ = sdl->sdl_type;
			switch (sdl->sdl_type) {
			case IFT_ETHER:
				*mp++ = ETHERCTL_MULTIADDRS;
				break;

			case IFT_FDDI:
				*mp++ = FDDICTL_MULTIADDRS;
				break;

			case IFT_ISO88025:
				*mp++ = TOKENCTL_MULTIADDRS;
				break;

			case IFT_IEEE80211:
				*mp++ = IEEE_802_11_SCTL_MULTIADDRS;
				break;

			default:
				return (NULL);
			}
			break;
#endif	/* CTL_LINK_LINKTYPE */

#ifdef	IPIFCTL_GEN_MULTIADDRS
		case PF_INET:
			*mp++ = CTL_LINK_PROTOTYPE;
			*mp++ = pf;
			*mp++ = 0; *mp++ = 0; *mp++ = 0; *mp++ = 0;
			*mp++ = IPIFCTL_GEN_MULTIADDRS;
			break;
#endif	/* IPIFCTL_GEN_MULTIADDRS */

		default:
			return (NULL);
		}

		if (sysctl(mib, mp - mib, NULL, &needed, NULL, 0) < 0) {
			_kvm_syserr(kd, kd->program, "kvm_ifmulti");
			return (NULL);
		}
		if ((buf = _kvm_malloc(kd, needed ? needed : 1)) == NULL)
			return (NULL);
		if (needed != 0 && 
		    sysctl(mib, mp - mib, buf, &needed, NULL, 0) < 0) {
			free(buf);
			_kvm_syserr(kd, kd->program, "kvm_ifmulti");
			return (NULL);
		}
		*rlen = needed;
		return (buf);
	}

	/* Simulate the sysctl */

	/* Read the if_index and verify the requested index is valid */
	if ((addr = _kvm_symbol(kd, "_if_index")) == 0 ||
	    kvm_read(kd, addr, &if_index, sizeof(if_index)) !=
	    sizeof(if_index))
		return (NULL);
	if (sdl->sdl_index > if_index) {
		_kvm_err(kd, kd->program, "no such interface");
		return (NULL);
	}

	i = if_index * sizeof(struct ifnet *);
	if ((ifnet_addrs = _kvm_malloc(kd, i)) == NULL)
		return (NULL);

	/* Find the ifnet_addrs array and read the ifaddr for the given index */
	if ((addr = _kvm_symbol(kd, "_ifnet_addrs")) == 0 ||
	    kvm_read(kd, addr, &ifap, sizeof(ifap)) != sizeof(ifap) ||
	    kvm_read(kd, (u_long)ifap, ifnet_addrs, i) != i ||
	    kvm_read(kd, (u_long)ifnet_addrs[sdl->sdl_index - 1], &ifaddr.ifa,
		sizeof(ifaddr.ifa)) != sizeof(ifaddr.ifa)) {
		free(ifnet_addrs);
		return (NULL);
	}
	free(ifnet_addrs);
	ifap = &ifaddr.ifa;

	switch (pf) {
	case PF_LINK:
		/* We only know about ARP capable interfaces */
		switch (sdl->sdl_type) {
		case IFT_ETHER:
		case IFT_FDDI:
		case IFT_ISO88025:
			break;
		default:
			return (NULL);
		}

		/* Fetch the arpcom and allocate space for returned info */
		if (kvm_read(kd, (u_long)ifap->ifa_ifp, &ac, sizeof(ac))
		    != sizeof(ac))
			return (NULL);

		if ((enm0 = enm = (struct ether_multi *)
		    _kvm_malloc(kd, ac.ac_multicnt ? ac.ac_multicnt *
			sizeof(*enm) : 1)) == NULL)
			return (NULL);

		if (ac.ac_multicnt < 1) {
			*rlen = 0;
			return (enm0);
		}

		/* Copy back the list of addresses */
		for (emp = ac.ac_multiaddrs; emp != NULL; emp = emp->enm_next) {
			if (kvm_read(kd, (u_long)emp, enm, sizeof(*enm)) !=
			    sizeof(*enm)) {
				free(enm0);
				return (NULL);
			}
			emp = enm++;
		}
		*rlen = (u_char *)enm - (u_char *)enm0;
		return (enm0);

	case PF_INET:
		/* Follow the address chain until we find the first in_ifaddr */
		while ((ifap = ifap->ifa_next) != NULL) {
			if (kvm_read(kd, (u_long)ifap, &ifaddr, 
			    sizeof(ifaddr)) != sizeof(ifaddr))
				return (NULL);
			ap = ADJSA(ifaddr.ifa.ifa_addr, ifap, &ifaddr.ifa);
			ifap = &ifaddr.ifa;
			if (ap->sa_family == AF_INET)
				break;
		}
		if (ifap == NULL) {
			*rlen = 0;
			return (_kvm_malloc(kd, 1));
		}

		/* Read the chain of struct in_multi into a linked list */
		inm1 = &inm0;
		for (i = 0, ia = ifaddr.in.ia_multiaddrs; ia != NULL; 
		     i++, ia = ia->inm_next) {
			if ((inm = _kvm_malloc(kd, sizeof(*inm))) == NULL) {
			free_list:
				*inm1 = NULL;
				do {
					inm0 = (inm = inm0)->inm_next;
					free(inm);
				} while (inm0 != NULL);
				return (NULL);
			}
			*inm1 = inm;
			inm1 = &inm->inm_next;
			if (kvm_read(kd, (u_long)ia, inm, sizeof(*inm)) != 
			    sizeof(*inm))
				goto free_list;
			ia = inm;
		}
		if (i == 0)
			goto free_list;

		/* Allocate array of in_multiaddr structures and fill in */
		if ((inma0 = inma = _kvm_malloc(kd, i * sizeof(*inma0))) == NULL)
			goto free_list;
		for (inm = inm0; inm != NULL; ) {
			inma->ima_group = inm->inm_addr;
			inma->ima_refcount = inm->inm_refcount;
			inma->ima_state = inm->inm_state;
			inma->ima_timer = inm->inm_timer;
			if ((inm = inm->inm_next) == NULL)
				inma->ima_next = NULL;
			else
				inma->ima_next = ++inma;
		}
		/* Free the in_multi structures */
		do {
			inm0 = (inm = inm0)->inm_next;
			free(inm);
		} while (inm0 != NULL);
		*rlen = (u_char *)inma - (u_char *)inma0;
		return (inma0);

	default:
		break;
	}

	return (NULL);
}

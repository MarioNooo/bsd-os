/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (C) 2000 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1993, 1994, 1995, 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ppp_ipcp.c,v 2.14 1996/12/31 23:59:30 prb Exp
 */

/*
 * This code is partially derived from CMU PPP.
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * PPP IPv6 Control Protocol
 */
#include <sys/types.h>
#include <sys/param.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/syslog.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/slcompress.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <unistd.h>

#include "pbuf.h"
#include "ppp.h"
#include "ppp_proto.h"
#include "ppp_var.h"

#if 0
static void   ppp_ipv6cp_setaddr(ppp_t *, struct sockaddr_in6 *, int);
#endif

void
ppp_ipv6cp_init(ppp)
	ppp_t *ppp;
{
	ppp->ppp_ncps++;
	ppp_ctl(ppp, PPP_SLCINIT);
	ppp_cp_up(ppp, FSM_IPV6CP);
}

int
ppp_ipv6cp_isup(ppp)
	ppp_t *ppp;
{
	return (ppp->ppp_ipv6cp.fsm_state == CP_OPENED ? 1 : 0);
}

int
ppp_ipv6cp_req(ppp)
	ppp_t *ppp;
{
	return (ppp_lcp_isup(ppp) && ppp_auth_isup(ppp));
}

/*
 * Timeout
 */
void
ppp_ipv6cp_timeout(ppp)
	void *ppp;
{
	ppp_cp_timeout((ppp_t *)ppp, FSM_IPV6CP);
}

/*
 * This level finished
 */
void
ppp_ipv6cp_tlf(ppp)
	ppp_t *ppp;
{
	/* If there is no more active NCPs drop the connection */
	if (--(ppp->ppp_ncps) == 0)
		ppp_down(ppp);
}

/*
 * IPCP reached CP_OPENED -- process the deferred queue
 */
void
ppp_ipv6cp_tlu(ppp)
	ppp_t *ppp;
{
	ppp_setparam(ppp, -1);		/* something might have changed */
	ppp_up(ppp);
	ppp_ctl(ppp, PPP_PROCDQ);
}

/*
 * Compose an IPCP Config-Request packet
 */
void
ppp_ipv6cp_creq(ppp)
	ppp_t *ppp;
{
	pbuf_t *m;
	pppopt_t opt;
	struct sockaddr_in6 a;

	dprintf(D_PKT, "BUILD IPV6CP_CREQ:\n");
	m = 0;

	/* IP-Compression-Protocol */
	if (!(ppp->ppp_ipv6cp.fsm_rej & (1<<IPV6CP_IFTOKEN)) &&
	    ppp_ipv6cp_getaddr(&a, ppp, 0) == 1) {
		dprintf(D_DT, "	{CPROT: %x}\n", PPP_IPV6CP);
		opt.po_type = IPV6CP_IFTOKEN;
		opt.po_len = 2 + sizeof(ppp->ppp_us6);
		memcpy(opt.po_data.c, &a.sin6_addr.s6_addr[8], 8);
		m = ppp_addopt(m, &opt);
	}

#if 0
	/* IP-Compression-Protocol */
	if (!(ppp->ppp_ipv6cp.fsm_rej & (1<<IPV6CP_CPROT)) &&
	    (ppp->ppp_flags & PPP_TCPC)) {
		dprintf(D_DT, "	{CPROT: %x}\n", PPP_VJC);
		opt.po_type = IPV6CP_CPROT;
		opt.po_len = 6;
		opt.po_data.s = htons(PPP_VJC);
		opt.po_data.c[2] = MAX_STATES - 1;
		opt.po_data.c[3] = 0;
		m = ppp_addopt(m, &opt);
	}
#endif

	m = ppp_cp_prepend(m, PPP_IPV6CP, CP_CONF_REQ, (++ppp->ppp_ipv6cp.fsm_id), 0);
	sendpacket(ppp, m);
}

/*
 * Process the rejected option
 */
void
ppp_ipv6cp_optrej(ppp, opt)
	ppp_t *ppp;
	pppopt_t *opt;
{
	switch (opt->po_type) {
	case IPV6CP_IFTOKEN:
		dprintf(D_DT, "	{IFTOKEN}\n");
		/* XXX */
		break;

#if 0
	case IPV6CP_CPROT:
		dprintf(D_DT, "	{CPROT}\n");
		/*
		 * indicate that peer can't send compressed
		 */
		ppp->ppp_flags &= ~PPP_TCPC;
		break;
#endif

	default:
		dprintf(D_DT, "	{%d: len=%d}\n", opt->po_type, opt->po_len);
	}
}

/*
 * Process the NAK-ed option
 */
void
ppp_ipv6cp_optnak(ppp, opt)
	ppp_t *ppp;
	pppopt_t *opt;
{
#if 0
	struct sockaddr_in6 a;
#endif

	switch (opt->po_type) {
	case IPV6CP_IFTOKEN:
		if (opt->po_len == 2 + sizeof(ppp->ppp_us6)) {
			dprintf(D_DT, "	{IFTOKEN: len=%d "
			    "ifid=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x}\n",
			    opt->po_len, opt->po_data.c[0], opt->po_data.c[1],
			    opt->po_data.c[2], opt->po_data.c[3],
			    opt->po_data.c[4], opt->po_data.c[5],
			    opt->po_data.c[6], opt->po_data.c[7]);
		} else
			dprintf(D_DT, "	{IFTOKEN: len=%d}\n", opt->po_len);
		/*XXX*/
		break;

#if 0
	case IPV6CP_CPROT:
		dprintf(D_DT, "	{CPROT: len=%d proto=0x%x}\n",
			opt->po_len, ntohs(opt->po_data.s));
		switch (ntohs(opt->po_data.s)) {
		case PPP_VJC:
			/*
			 * indicate that peer can't send compressed
			 */
			dprintf(D_DT, "	[Disable VJC]\n");
			ppp->ppp_flags &= ~PPP_TCPC;
			break;
		default:
			dprintf(D_DT, "	[Disable UNKNOWN]\n");
			break;
		}
		break;
#endif

	default:
		dprintf(D_DT, "	{%d: len=%d}\n", opt->po_type, opt->po_len);
	}
}

/*
 * Process the received option
 */
int
ppp_ipv6cp_opt(ppp, opt)
	ppp_t *ppp;
	pppopt_t *opt;
{
	struct sockaddr_in6 a;
	int res;

	switch (opt->po_type) {
	default:
		dprintf(D_DT, "	{UNKNOWN: type=%d len=%d}\n",
		    opt->po_type, opt->po_len);
		return (OPT_REJ);

	case IPV6CP_IFTOKEN:        /* Interface-Token */
		dprintf(D_DT, "	{IFTOKEN: len=%d}\n", opt->po_len);
		if (opt->po_len != 2 + sizeof(ppp->ppp_us6)) {
			dprintf(D_DT, "	{ADDRS: len=%d (should be %d)}\n",
			    opt->po_len, 2 + sizeof(ppp->ppp_us6));
			return (OPT_REJ);
		}
		dprintf(D_DT, "	{IFTOKEN: len=%d, "
		    "ifid=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x}\n",
		    opt->po_len, opt->po_data.c[0], opt->po_data.c[1],
		    opt->po_data.c[2], opt->po_data.c[3], opt->po_data.c[4],
		    opt->po_data.c[5], opt->po_data.c[6], opt->po_data.c[7]);

		res = OPT_OK;

		/* Handle the destination's ifid */

		if (ppp_ipv6cp_getaddr(&a, ppp, 0) == 1 &&
		    !memcmp(&a.sin6_addr.s6_addr[8], &opt->po_data.c[0], 8)) {
			/* ifid collision, suggest different ifid */
			memcpy(&opt->po_data.c[0], &a.sin6_addr.s6_addr[8], 8);
			opt->po_data.c[0] &= ~0x02;	/*u bit to "local"*/
			opt->po_data.c[6] ^= 0xff;
			opt->po_data.c[7] ^= 0xff;
			res = OPT_NAK;
		} else {
			/* Our ifid undefined, or no collision -- don't care */
		}

		return (res);

#if 0
	case IPV6CP_CPROT:        /* IP-Compression */
		dprintf(D_DT, "	{CPROT: len=%d prot=0x%x ms=%d c=%d}\n",
		    opt->po_len, ntohs(opt->po_data.s),
		    opt->po_data.c[2], opt->po_data.c[3]);
		if (opt->po_len != 4 && opt->po_len != 6)
			return (OPT_REJ);
		if (!(ppp->ppp_flags & PPP_TCPC))
			return (OPT_REJ);
		res = OPT_OK;
		if (ntohs(opt->po_data.s) != PPP_VJC) {
			opt->po_data.s = htons(PPP_VJC);
			res = OPT_NAK;
		}
		if (opt->po_len == 6) {         /* rfc1332? */
			if (opt->po_data.c[2] >= MAX_STATES) {
				opt->po_data.c[2] = MAX_STATES - 1;
				res = OPT_NAK;
			}
			if (opt->po_data.c[3] != 0) {
				opt->po_data.c[3] = 0;
				res = OPT_NAK;
			}
		}
		if (res == OPT_OK)
			ppp->ppp_txflags |= PPP_TCPC;
		return (res);
#endif
	}
	/*NOTREACHED*/
}

/*
 * Get IP address from the address list from interface.
 * Returns  1 if there is an address assigned to the interface.
 * Returns  0 if there is no address.
 * Returns -1 if interface could not be found.
 */
int
ppp_ipv6cp_retaddrs(ppp, src, dst)
	ppp_t *ppp;
	struct sockaddr_in6 *src;
	struct sockaddr_in6 *dst;
{
	struct ifaddrs *ifa0, *ifa;
	int retval = -1;
	char *ifname;

	ifname = *ppp->ppp_pifname ? ppp->ppp_pifname : ppp->ppp_ifname;

	if (getifaddrs(&ifa0) != 0) {
		syslog(LOG_WARNING|LOG_PERROR,
		    "%s: unable to read interface configuration: %m",
		    ifname);
		free(ifa0);
		return (retval);
	}

	for (ifa = ifa0; ifa != NULL; ifa = ifa->ifa_next) {
		if (strcmp(ifname, ifa->ifa_name) != 0) {
			continue;
		}
		retval = 0;

		if (ifa->ifa_addr == NULL ||
		    ifa->ifa_addr->sa_family != AF_INET6) {
			continue;
		}
		if (!IN6_IS_ADDR_LINKLOCAL(&((struct sockaddr_in6 *)
		    ifa->ifa_addr)->sin6_addr)) {
			continue;
		}
	   	*src = *((struct sockaddr_in6 *)ifa->ifa_addr);
		if (ifa->ifa_dstaddr)
			*dst = *((struct sockaddr_in6 *)ifa->ifa_dstaddr);
		else
			memset(dst, 0, sizeof(*dst));
		retval = 1;
		break;
	}
	return(retval);
}

int
ppp_ipv6cp_getaddr(addr, ppp, dst)
	struct sockaddr_in6 *addr;
        ppp_t *ppp;
        int dst;
{
	struct sockaddr_in6 srcaddr;
	struct sockaddr_in6 dstaddr;
	char h[MAXHOSTNAMELEN];

	if (ppp_ipv6cp_retaddrs(ppp, &srcaddr, &dstaddr) == 1) {
		memcpy(addr, (dst == DST_ADDR ? &dstaddr : &srcaddr),
		    sizeof(*addr));
		return 1;
	}

	memset(addr, 0, sizeof(*addr));
	addr->sin6_family = AF_INET6;
	addr->sin6_len = sizeof(struct sockaddr_in6);
	addr->sin6_addr.s6_addr[0] = 0xfe;
	addr->sin6_addr.s6_addr[1] = 0x80;
#if 0
	memcpy(&addr->sin6_addr.s6_addr[8],
	    (dst == DST_ADDR ? &ppp->ppp_them6 : &ppp->ppp_us6), 8);
#else
	gethostname(h, sizeof(h));
	memcpy(&addr->sin6_addr.s6_addr[8], crypt(h, h), 8);
	addr->sin6_addr.s6_addr[8] &= ~0x02;	/*u bit to "local"*/
#endif
	/* XXX scopeid */
	return 1;
}

#if 0
/*
 * Set remote interface address
 */
static void
ppp_ipv6cp_setaddr(ppp, a, dst)
	ppp_t *ppp;
	struct sockaddr_in6 *a;
	int dst;
{
	struct in6_aliasreq ifra;
	struct in6_ifreq ifr;
	char *ifname;
	struct sockaddr_in6 srcaddr, dstaddr;

	ifname = ppp->ppp_pifname[0] ? ppp->ppp_pifname : ppp->ppp_ifname;

	memset(&ifra, 0, sizeof(ifra));
	strncpy(ifra.ifra_name, ifname, sizeof (ifra.ifra_name));
	ifra.ifra_addr.sin6_len = sizeof(struct sockaddr_in6);
	ifra.ifra_addr.sin6_family = AF_INET6;
	ifra.ifra_dstaddr.sin6_len = sizeof(struct sockaddr_in6);
	ifra.ifra_dstaddr.sin6_family = AF_INET6;

	if (dst == DST_ADDR)
		ifra.ifra_dstaddr = *a;
	else {
		ifra.ifra_addr = *a;
		ppp_ipv6cp_getaddr(&ifra.ifra_dstaddr, ppp, DST_ADDR);
	}

	/*
	 * Remove  address if it is not for this source.
	 * We expect only one address on the interface.
	 */
	if (dst == SRC_ADDR &&
	    ppp_ipv6cp_retaddrs(ppp, &srcaddr, &dstaddr) == 1 &&
	    !IN6_ARE_ADDR_EQUAL(&srcaddr.sin6_addr, &a->sin6_addr)) {
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
		ifr.ifr_addr = srcaddr;
		if (ioctl(skt, SIOCDIFADDR_IN6, (caddr_t) &ifr) < 0)
			warn("%s: deleting %s", dst == DST_ADDR ?
			    "dst" : "src", ifname);
	}

	if (ioctl(skt, SIOCAIFADDR_IN6, (caddr_t) &ifra) < 0)
		warn("%s: setting %s", dst == DST_ADDR ? "dst" : "src", ifname);
}
#endif

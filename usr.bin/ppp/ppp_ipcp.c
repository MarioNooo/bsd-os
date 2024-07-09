/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1993, 1994, 1995, 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ppp_ipcp.c,v 2.23 2001/10/03 17:29:57 polk Exp
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
 * PPP IP Control Protocol
 */
#include <sys/types.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/queue.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/slcompress.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <ifaddrs.h>

#include "pbuf.h"
#include "ppp.h"
#include "ppp_proto.h"
#include "ppp_var.h"

static void   ppp_ipcp_setaddr(ppp_t *, u_long, int);

void
ppp_ipcp_init(ppp)
	ppp_t *ppp;
{
	ppp->ppp_ncps++;
	ppp_ctl(ppp, PPP_SLCINIT);
	ppp_cp_up(ppp, FSM_IPCP);
}

int
ppp_ipcp_isup(ppp)
	ppp_t *ppp;
{
	return (ppp->ppp_ipcp.fsm_state == CP_OPENED ? 1 : 0);
}

int
ppp_ipcp_req(ppp)
	ppp_t *ppp;
{
	return (ppp_lcp_isup(ppp) && ppp_auth_isup(ppp));
}

/*
 * Timeout
 */
void
ppp_ipcp_timeout(ppp)
	void *ppp;
{
	ppp_cp_timeout((ppp_t *)ppp, FSM_IPCP);
}

/*
 * This level finished
 */
void
ppp_ipcp_tlf(ppp)
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
ppp_ipcp_tlu(ppp)
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
ppp_ipcp_creq(ppp)
	ppp_t *ppp;
{
	pbuf_t *m;
	pppopt_t opt;
	u_long a;

	dprintf(D_PKT, "BUILD IPCP_CREQ:\n");
	m = 0;

	/* IP-Compression-Protocol */
	if (!(ppp->ppp_ipcp.fsm_rej & (1<<IPCP_CPROT)) &&
	    (ppp->ppp_flags & PPP_TCPC)) {
		dprintf(D_DT, "	{CPROT: %x}\n", PPP_VJC);
		opt.po_type = IPCP_CPROT;
		opt.po_len = 6;
		opt.po_data.s = htons(PPP_VJC);
		opt.po_data.c[2] = MAX_STATES - 1;
		opt.po_data.c[3] = 0;
		m = ppp_addopt(m, &opt);
	}

	/* IP-Address */
	if (!(ppp->ppp_ipcp.fsm_rej & (1<<IPCP_ADDR))) {
		a = ppp_ipcp_getaddr(ppp, SRC_ADDR);
		if (a == 0)
			a = ppp->ppp_us;
		dprintf(D_DT, "	{ADDR: %d.%d.%d.%d}\n", ntohl(a)>>24,
			(ntohl(a)>>16) & 0xff, (ntohl(a)>>8) & 0xff, ntohl(a) & 0xff);
		opt.po_type = IPCP_ADDR;
		opt.po_len = 6;
		opt.po_data.l = a;
		m = ppp_addopt(m, &opt);
	}

	m = ppp_cp_prepend(m, PPP_IPCP, CP_CONF_REQ, (++ppp->ppp_ipcp.fsm_id), 0);
	sendpacket(ppp, m);
}

/*
 * Process the rejected option
 */
void
ppp_ipcp_optrej(ppp, opt)
	ppp_t *ppp;
	pppopt_t *opt;
{
	switch (opt->po_type) {
	case IPCP_CPROT:
		dprintf(D_DT, "	{CPROT}\n");
		/*
		 * indicate that peer can't send compressed
		 */
		ppp->ppp_flags &= ~PPP_TCPC;
		break;

	default:
		dprintf(D_DT, "	{%d: len=%d}\n", opt->po_type, opt->po_len);
	}
}

/*
 * Process the NAK-ed option
 */
void
ppp_ipcp_optnak(ppp, opt)
	ppp_t *ppp;
	pppopt_t *opt;
{
	u_long a;

	switch (opt->po_type) {
	case IPCP_CPROT:
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

	case IPCP_ADDR:
		dprintf(D_DT, "	{ADDR: len=%d addr=%d.%d.%d.%d}\n",
			opt->po_len, opt->po_data.c[0],
			opt->po_data.c[1], opt->po_data.c[2],
			opt->po_data.c[3]);
		if ((ppp->ppp_flags & PPP_CHGADDR) && opt->po_data.l)
			a = 0;
		else
			a = ppp_ipcp_getaddr(ppp, SRC_ADDR);
		if (a != 0) {
			if (a != opt->po_data.l) {
				printf("ppp: cannot negotiate local IP address,"
				    "peer requests %d.%d.%d.%d\n",
					opt->po_data.c[0], opt->po_data.c[1],
					opt->po_data.c[2], opt->po_data.c[3]);
				ppp_cp_close(ppp, FSM_IPCP);
			}
		} else {
			dprintf(D_DT, "	*SET SRC ADDR*\n");
			ppp_ipcp_setaddr(ppp, opt->po_data.l, 0);
		}
		break;

	default:
		dprintf(D_DT, "	{%d: len=%d}\n", opt->po_type, opt->po_len);
	}
}

/*
 * Process the received option
 */
int
ppp_ipcp_opt(ppp, opt)
	ppp_t *ppp;
	pppopt_t *opt;
{
	u_long a, b;
	int res;

	switch (opt->po_type) {
	default:
		dprintf(D_DT, "	{UNKNOWN: type=%d len=%d}\n",
		    opt->po_type, opt->po_len);
		return (OPT_REJ);

	case IPCP_ADDRS:        /* IP-Addresses */
		dprintf(D_DT, "	{ADDRS: len=%d}\n", opt->po_len);
		if (opt->po_len != 2 + 2 * sizeof(long)) {
			dprintf(D_DT, "	{ADDRS: len=%d (should be %d)}\n",
			    opt->po_len,2 + 2 * sizeof(long));
			return (OPT_REJ);
		}
		dprintf(D_DT, "	{ADDRS: len=%d, dst=%d.%d.%d.%d, "
		    "src=%d.%d.%d.%d}\n",
		    opt->po_len,
		    opt->po_data.c[0], opt->po_data.c[1],
		    opt->po_data.c[2], opt->po_data.c[3],
		    opt->po_data.c[4], opt->po_data.c[5],
		    opt->po_data.c[6], opt->po_data.c[7]);

		res = OPT_OK;

		/* Handle the destination's address */

		a = ppp_ipcp_getaddr(ppp, DST_ADDR);
		if (a == 0) {   /* Our dst addr undefined -- accept peer's */
			dprintf(D_DT, "	*SET DST ADDR <to %d.%d.%d.%d>*\n",
			    opt->po_data.c[0], opt->po_data.c[1],
			    opt->po_data.c[2], opt->po_data.c[3]);
			ppp_ipcp_setaddr(ppp, opt->po_data.l, 1);
		} else if (opt->po_data.l != a) {
			dprintf(D_DT, "	*CHG DST ADDR <from %d.%d.%d.%d\n",
			    opt->po_data.c[0], opt->po_data.c[1],
			    opt->po_data.c[2], opt->po_data.c[3]);
			opt->po_data.l = a;
			dprintf(D_DT, "	to %d.%d.%d.%d>\n",
			    opt->po_data.c[0], opt->po_data.c[1],
			    opt->po_data.c[2], opt->po_data.c[3]);
			res = OPT_NAK;
		}

		/* Handle what the destination thinks our address is */

		if ((ppp->ppp_flags & PPP_CHGADDR) && (&opt->po_data.l)[1])
			a = 0;
		else
			a = ppp_ipcp_getaddr(ppp, SRC_ADDR);
		b = (&opt->po_data.l)[1];
		if (a == 0) {   /* Our src addr undefined -- accept peer's */
			dprintf(D_DT, "	*SET SRC ADDR <to %d.%d.%d.%d>*\n",
			    opt->po_data.c[4], opt->po_data.c[5],
			    opt->po_data.c[6], opt->po_data.c[7]);
			ppp_ipcp_setaddr(ppp, b, 0);
		} else if (b != a) {
			dprintf(D_DT, "	*WRONG SRC ADDR <from %d.%d.%d.%d",
			    opt->po_data.c[4], opt->po_data.c[5],
			    opt->po_data.c[6], opt->po_data.c[7]);
			(&opt->po_data.l)[1] = a;
			dprintf(D_DT, " to %d.%d.%d.%d>\n",
			    opt->po_data.c[4], opt->po_data.c[5],
			    opt->po_data.c[6], opt->po_data.c[7]);
			res = OPT_NAK;
		}

		return (res);

	case IPCP_CPROT:        /* IP-Compression */
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

	case IPCP_ADDR:
		dprintf(D_DT, "	{ADDR: len=%d addr=%d.%d.%d.%d}\n",
		    opt->po_len,
		    opt->po_data.c[0], opt->po_data.c[1],
		    opt->po_data.c[2], opt->po_data.c[3]);
		if (opt->po_len != 6)
			return (OPT_REJ);


		/*
		 * If our dst addr is undefined, or allow-addr-change
		 * then accept peer's
		 */
		if ((ppp->ppp_flags & PPP_CHGADDR) && opt->po_data.l)
			a = 0;
		else
			a = ppp_ipcp_getaddr(ppp, DST_ADDR);

		if (a == 0) {
			dprintf(D_DT, "	*SET DST ADDR*\n");
			ppp_ipcp_setaddr(ppp, opt->po_data.l, 1);
		} else if (opt->po_data.l != a) {
			opt->po_data.l = a;
			return (OPT_NAK);
		}
		return (OPT_OK);

	case IPCP_MS_PDNS:
	case IPCP_MS_SDNS:
		dprintf(D_DT, "	{MS_%cDNS: len=%d addr=%d.%d.%d.%d}\n",
		    opt->po_type == IPCP_MS_PDNS ? 'P' : 'S',
		    opt->po_len,
		    opt->po_data.c[0], opt->po_data.c[1],
		    opt->po_data.c[2], opt->po_data.c[3]);
		if (opt->po_len != 6)
			return (OPT_REJ);
		a = opt->po_type == IPCP_MS_PDNS ? 0 : 1;
		if (ppp->ppp_dns[a] == 0)
			return (OPT_REJ);
		if (opt->po_data.l != ppp->ppp_dns[a]) {
			opt->po_data.l = ppp->ppp_dns[a];
			return (OPT_NAK);
		}
		return (OPT_OK);

	case IPCP_MS_PNBS:
	case IPCP_MS_SNBS:
		dprintf(D_DT, "	{MS_%cNBS: len=%d addr=%d.%d.%d.%d}\n",
		    opt->po_type == IPCP_MS_PNBS ? 'P' : 'S',
		    opt->po_len,
		    opt->po_data.c[0], opt->po_data.c[1],
		    opt->po_data.c[2], opt->po_data.c[3]);
		if (opt->po_len != 6)
			return (OPT_REJ);
		a = IPCP_MS_PNBS ? 0 : 1;
		if (ppp->ppp_nbs[a] == 0)
			return (OPT_REJ);
		if (opt->po_data.l != ppp->ppp_nbs[a]) {
			opt->po_data.l = ppp->ppp_nbs[a];
			return (OPT_NAK);
		}
		return (OPT_OK);

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
ppp_ipcp_retaddrs(ppp, src, dst)
	ppp_t *ppp;
	u_long *src;
	u_long *dst;
{
	struct ifaddrs *ifa0, *ifa;
	int retval = -1;
	char *ifname;

	ifname = *ppp->ppp_pifname ? ppp->ppp_pifname : ppp->ppp_ifname;

	if (getifaddrs(&ifa0) != 0) {
		syslog(LOG_WARNING|LOG_PERROR,
		    "%s: unable to read interface configuration: %m",
		    ifname);
		return (retval);
	}

	for (ifa = ifa0; ifa != NULL; ifa = ifa->ifa_next) {
		if (strcmp(ifname, ifa->ifa_name) != 0) {
			continue;
		}
		retval = 0;

		if (ifa->ifa_addr == NULL ||
		    ifa->ifa_addr->sa_family != AF_INET) {
			continue;
		}
		*dst = ((struct sockaddr_in *)ifa->ifa_dstaddr)->sin_addr.s_addr;
	   	*src = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr;
		retval = 1;
		break;
	}
	freeifaddrs(ifa0);
	return(retval);
}

u_long
ppp_ipcp_getaddr(ppp, dst)
        ppp_t *ppp;
        int dst;
{
	u_long srcaddr;
	u_long dstaddr;

	if (ppp_ipcp_retaddrs(ppp, &srcaddr, &dstaddr) == 1)
		return(dst == DST_ADDR ? dstaddr : srcaddr);

	return (dst == DST_ADDR ? ppp->ppp_them : ppp->ppp_us);
}

/*
 * Set remote interface address
 */
static void
ppp_ipcp_setaddr(ppp, a, dst)
	ppp_t *ppp;
	u_long a;
	int dst;
{
	struct in_aliasreq ifra;
	struct ifreq ifr;
	char *ifname;
	u_long srcaddr, dstaddr;

	ifname = ppp->ppp_pifname[0] ? ppp->ppp_pifname : ppp->ppp_ifname;

	memset(&ifra, 0, sizeof(ifra));
	strncpy(ifra.ifra_name, ifname, sizeof (ifra.ifra_name));
	ifra.ifra_addr.sin_len = sizeof(ifra.ifra_addr);
	ifra.ifra_addr.sin_family = AF_INET;
	ifra.ifra_dstaddr.sin_len = sizeof(ifra.ifra_dstaddr);
	ifra.ifra_dstaddr.sin_family = AF_INET;

	if (dst == DST_ADDR)
		ifra.ifra_dstaddr.sin_addr.s_addr = a;
	else {
		ifra.ifra_addr.sin_addr.s_addr = a;
		ifra.ifra_dstaddr.sin_addr.s_addr = ppp_ipcp_getaddr(ppp,
		    DST_ADDR);
	}

	/*
	 * Remove  address if it is not for this source.
	 * We expect only one address on the interface.
	 */
	if (dst == SRC_ADDR &&
	    ppp_ipcp_retaddrs(ppp, &srcaddr, &dstaddr) == 1 &&
	    srcaddr != a ) {
			memset(&ifr, 0, sizeof(ifr));
			strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
			ifr.ifr_addr.sa_len = sizeof(struct sockaddr_in);
			ifr.ifr_addr.sa_family = AF_INET;
			if (ioctl(skt, SIOCDIFADDR, (caddr_t) &ifr) < 0)
				warn("%s: deleting %s", dst == DST_ADDR ?
				    "dst" : "src", ifname);
	}

	if (ioctl(skt, SIOCAIFADDR, (caddr_t) &ifra) < 0)
		warn("%s: setting %s", dst == DST_ADDR ? "dst" : "src", ifname);
}

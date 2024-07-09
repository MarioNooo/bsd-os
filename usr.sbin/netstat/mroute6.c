/*
 * Copyright (C) 1998 WIDE Project.
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

/*
 * Copyright (c) 1989 Stephen Deering
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Stephen Deering of Stanford University.
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
 *	@(#)mroute.c	8.2 (Berkeley) 4/28/95
 */
#define NEW_STRUCT_ROUTE

/*
 * We need the normally kernel-only struct selinfo definition
 */
#define _NEED_STRUCT_SELINFO

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>

#include <stdio.h>
#include <err.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>

#define KERNEL 1
#include <netinet6/ip6_mroute.h>
#undef KERNEL

#include <netdb.h>

#include "netstat.h"

#if 0				/* XXX */
#define	WID_ORG	(lflag ? 39 : (nflag ? 29 : 18)) /* width of origin column */
#define	WID_GRP	(lflag ? 18 : (nflag ? 16 : 18)) /* width of group column */
#else
#define	WID_ORG	39 /* width of origin column */
#define	WID_GRP	18 /* width of group column */
#endif

static u_int mrtproto;
static struct data_info mrt6proto_info = {
    "_ip6_mrtproto",
    NULL, 0,
    &mrtproto, sizeof(mrtproto)
};

static struct mf6c *mf6ctable[MF6CTBLSIZ];
static struct data_info mf6caddr_info = {
    "_mf6ctable",
    NULL, 0,
    mf6ctable, sizeof(mf6ctable)
};

static struct mif6 mif6table[MAXMIFS];
static struct data_info mifaddr_info = {
    "_mif6table",
    NULL, 0,
    mif6table, sizeof(mif6table)
};

void
mroute6pr()
{
	struct mf6c mfc, *mfcp;
	struct rtdetq rte, *rtep;
	register struct mif6 *mifp;
	register mifi_t mifi;
	register int i;
	register int banner_printed;
	register int saved_nflag;
	mifi_t maxmif = 0;
	long waitings;
	char srcbuf[NI_MAXHOST], grpbuf[NI_MAXHOST];
#ifdef NI_WITHSCOPEID
	const int niflags = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
	const int niflags = NI_NUMERICHOST;
#endif

	if (skread("IPv6 multicast routing proto", &mrt6proto_info))
		return;

	switch (mrtproto) {

	 case 0:
		 printf("no IPv6 multicast routing compiled into this system\n");
		 return;

	 case IPPROTO_PIM:
		 break;

	 default:
		 printf("IPv6 multicast routing protocol %u, unknown\n",
			mrtproto);
		 return;
	}

	if (skread("IPv6 multicast routing interface", &mifaddr_info))
		return;

	saved_nflag = nflag;
	nflag = 1;

	banner_printed = 0;
	for (mifi = 0, mifp = mif6table; mifi < MAXMIFS; ++mifi, ++mifp) {
		struct ifnet ifnet;
		char ifname[IFNAMSIZ];

		if (mifp->m6_ifp == NULL)
			continue;

		kread((u_long)mifp->m6_ifp, (char *)&ifnet, sizeof(ifnet));
		maxmif = mifi;
		if (!banner_printed) {
			printf("\nIPv6 Multicast Interface Table\n"
			       " Mif   Rate   PhyIF   "
			       "Pkts-In   Pkts-Out\n");
			banner_printed = 1;
		}

		printf("  %2u   %4d",
		       mifi, mifp->m6_rate_limit);
		printf("   %5s", (mifp->m6_flags & MIFF_REGISTER) ?
		       "reg0" : if_indextoname(ifnet.if_index, ifname));

		printf(" %9qu  %9qu\n", mifp->m6_pkt_in, mifp->m6_pkt_out);
	}
	if (!banner_printed)
		printf("\nIPv6 Multicast Interface Table is empty\n");

	if (skread("IPv6 multicast routing", &mf6caddr_info))
		return;

	banner_printed = 0;
	for (i = 0; i < MF6CTBLSIZ; ++i) {
		struct sockaddr_in6 sasrc, sagrp;

		mfcp = mf6ctable[i];
		while(mfcp) {
			kread((u_long)mfcp, (char *)&mfc, sizeof(mfc));
			if (!banner_printed) {
				printf ("\nIPv6 Multicast Forwarding Cache\n");
				printf(" %-*.*s %-*.*s %s",
				       WID_ORG, WID_ORG, "Origin",
				       WID_GRP, WID_GRP, "Group",
				       "  Packets Waits In-Mif  Out-Mifs\n");
				banner_printed = 1;
			}

			memset(&sasrc, 0, sizeof(sasrc));
			memset(&sagrp, 0, sizeof(sagrp));
			sasrc.sin6_family = sagrp.sin6_family = AF_INET6;
			sasrc.sin6_len = sagrp.sin6_len = sizeof(sasrc);
			sasrc.sin6_addr = mfc.mf6c_origin.sin6_addr;
			sagrp.sin6_addr = mfc.mf6c_mcastgrp.sin6_addr;

			if (getnameinfo((struct sockaddr *)&sasrc,
					sizeof(sasrc), srcbuf,
					sizeof(srcbuf), NULL, 0,
					nflag ? niflags : 0)) {
				warnx("mroute6pr: getnameinfo for src failed");
				goto nextcache;
			}
			if (getnameinfo((struct sockaddr *)&sagrp,
					sizeof(sagrp), grpbuf,
					sizeof(grpbuf), NULL, 0,
					nflag ? niflags : 0)) {
				warnx("mroute6pr: getnameinfo for grp failed");
				goto nextcache;
			}
			
			printf(" %-*.*s", WID_ORG, WID_ORG, srcbuf);
			printf(" %-*.*s", WID_GRP, WID_GRP, grpbuf);
			printf(" %9qu", mfc.mf6c_pkt_cnt);

			for (waitings = 0, rtep = mfc.mf6c_stall; rtep; ) {
				waitings++;
				kread((u_long)rtep, (char *)&rte, sizeof(rte));
				rtep = rte.next;
			}
			printf("   %3ld", waitings);

			if (mfc.mf6c_parent == MF6C_INCOMPLETE_PARENT)
				printf("  ---   ");
			else
				printf("  %3d   ", mfc.mf6c_parent);
			for (mifi = 0; mifi <= MAXMIFS; mifi++) {
				if (IF_ISSET(mifi, &mfc.mf6c_ifset))
					printf(" %u", mifi);
			}

		  nextcache:
			printf("\n");

			mfcp = mfc.mf6c_next;
		}
	}
	if (!banner_printed)
		printf("\nIPv6 Multicast Routing Table is empty\n");

	printf("\n");
	nflag = saved_nflag;
}

static struct mrt6stat mrtstat;
static struct data_info mrt6stat_info = {
    "_mrt6stat",
    NULL, 0,
    &mrtstat, sizeof(mrtstat)
};

void
mrt6_stats()
{
	u_int mrtproto;
	struct mrt6stat mrtstat;

	if (skread("IPv6 multicast stats", &mrt6proto_info) ||
	    skread("IPv6 multicast stats", &mrt6stat_info))
		return;

	switch (mrtproto) {
	 case 0:
		 printf("no IPv6 multicast routing compiled into this system\n");
		 return;

	 case IPPROTO_PIM:
		 break;

	 default:
		 printf("IPv6 multicast routing protocol %u, unknown\n",
			mrtproto);
		 return;
	}

	printf("multicast forwarding:\n");
	printf(" %10qu multicast forwarding cache lookup%s\n",
	       mrtstat.mrt6s_mfc_lookups, PLURAL(mrtstat.mrt6s_mfc_lookups));
	printf(" %10qu multicast forwarding cache miss%s\n",
	       mrtstat.mrt6s_mfc_misses, PLURALES(mrtstat.mrt6s_mfc_misses));
	printf(" %10qu upcall%s to mrouted\n",
	       mrtstat.mrt6s_upcalls, PLURAL(mrtstat.mrt6s_upcalls));
	printf(" %10qu upcall queue overflow%s\n",
	  mrtstat.mrt6s_upq_ovflw, PLURAL(mrtstat.mrt6s_upq_ovflw));
	printf(" %10qu upcall%s dropped due to full socket buffer\n",
	  mrtstat.mrt6s_upq_sockfull, PLURAL(mrtstat.mrt6s_upq_sockfull));
	printf(" %10qu cache cleanup%s\n",
	  mrtstat.mrt6s_cache_cleanups, PLURAL(mrtstat.mrt6s_cache_cleanups));
	printf(" %10qu datagram%s with no route for origin\n",
	  mrtstat.mrt6s_no_route, PLURAL(mrtstat.mrt6s_no_route));
	printf(" %10qu datagram%s arrived with bad tunneling\n",
	  mrtstat.mrt6s_bad_tunnel, PLURAL(mrtstat.mrt6s_bad_tunnel));
	printf(" %10qu datagram%s could not be tunneled\n",
	  mrtstat.mrt6s_cant_tunnel, PLURAL(mrtstat.mrt6s_cant_tunnel));
	printf(" %10qu datagram%s arrived on wrong interface\n",
	  mrtstat.mrt6s_wrong_if, PLURAL(mrtstat.mrt6s_wrong_if));
	printf(" %10qu datagram%s selectively dropped\n",
	  mrtstat.mrt6s_drop_sel, PLURAL(mrtstat.mrt6s_drop_sel));
	printf(" %10qu datagram%s dropped due to queue overflow\n",
	  mrtstat.mrt6s_q_overflow, PLURAL(mrtstat.mrt6s_q_overflow));
	printf(" %10qu datagram%s dropped for being too large\n",
	  mrtstat.mrt6s_pkt2large, PLURAL(mrtstat.mrt6s_pkt2large));
}

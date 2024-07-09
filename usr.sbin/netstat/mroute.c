/*	BSDI mroute.c,v 2.10 1997/10/31 04:13:40 jch Exp	*/
/*
 * Print DVMRP multicast routing structures and statistics.
 *
 * MROUTING Revision: 3.5
 */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/igmp.h>
#define KERNEL 1
#include <netinet/ip_mroute.h>
#undef KERNEL

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include "netstat.h"

static char *
pktscale(int n)
{
	static char buf[6];
	char t;

	if (n < 1024) {
		t = ' ';
	} else if (n < 1048576) {
		t = 'k';
		n /= 1024;
	} else {
		t = 'm';
		n /= 1048576;
	}

	(void)sprintf(buf, "%4u%c", n, t);

	return buf;
}

static u_int mrtproto;
static int mrtproto_mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_MRTPROTO };
static struct data_info mrtproto_info = {
    "_ip_mrtproto",
    mrtproto_mib, sizeof(mrtproto_mib)/sizeof(*mrtproto_mib),
    &mrtproto, sizeof(mrtproto)
};

static struct mfc *mfctable[MFCTBLSIZ];
static int mfctable_mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_MRTTABLE };
static struct data_info mfcaddr_info = {
    "_mfctable",
    mfctable_mib, sizeof(mfctable_mib)/sizeof(*mfctable_mib),
    mfctable, sizeof(mfctable)
};

static struct vif viftable[MAXVIFS];
static int viftable_mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_VIFTABLE };
static struct data_info vifaddr_info = {
    "_viftable",
    viftable_mib, sizeof(viftable_mib)/sizeof(*viftable_mib),
    viftable, sizeof(viftable)
};

void
mroutepr(void)
{
	struct mfc **mfctab, *mfcp, mfc;
	struct vif *v;
	vifi_t vifi;
	size_t needed;
	int i;
	int banner_printed;
	int numvifs;
	int nmfc;			/* No. of cache entries */

	if (skread("multicast routing proto", &mrtproto_info))
		return;

	switch (mrtproto) {
	    case 0:
		warnx("multicast routing not supported by this system");
		return;

	    case IGMP_DVMRP:
		break;

	    default:
		warnx("multicast routing protocol %u, unknown", mrtproto);
		return;
	}

	if (skread("multicast routing", &vifaddr_info))
		return;
	banner_printed = 0;
	numvifs = 0;
	
	for (vifi = 0, v = viftable; vifi < MAXVIFS; ++vifi, ++v) {

		if (v->v_lcl_addr.s_addr == 0) 
			continue;

		numvifs = vifi;
		
		if (!banner_printed) {
			printf("\nVirtual Interface Table\n%s%s%s",
			    " Vif  Thresh  Rate_lmt  ",
			    "Local-Address   ",
			    "Remote-Address     Pkt_in   Pkt_out\n");
			banner_printed = 1;
		}

		printf(" %2u    %3u   %6u     %-15.15s",
		    vifi, v->v_threshold, 
		    v->v_rate_limit, routename(v->v_lcl_addr.s_addr));
		printf(" %-15.15s  %8lu  %8lu\n",
		    (v->v_flags & VIFF_TUNNEL) ?
		    routename(v->v_rmt_addr.s_addr) : filler,
		    v->v_pkt_in, v->v_pkt_out);
	}
	if (!banner_printed)
		printf("\nVirtual Interface Table is empty\n");

	if (!kflag) {
		if (sysctl(mfcaddr_info.di_mib, mfcaddr_info.di_miblen, 
		    NULL, &needed, NULL, 0) < 0)
			err(1, "%s", mfcaddr_info.di_name);
		if ((mfctab = (struct mfc **)malloc(needed)) == NULL)
			errx(1, "malloc: Out of memory");
		if (sysctl(mfcaddr_info.di_mib, mfcaddr_info.di_miblen, mfctab,
		    &needed, NULL, 0) < 0) {
			free(mfctab);
			err(1, "%s", mfcaddr_info.di_name);
		}
	} else {
		if (skread("multicast routing", &mfcaddr_info))
			return;
		mfctab = mfctable;
	}

	banner_printed = 0;
	for (i = 0, nmfc = 0; i < MFCTBLSIZ; ++i) {
		for (mfcp = mfctab[i]; mfcp != NULL; mfcp = mfcp->mfc_next) {

			if (!banner_printed) {
				printf("\nMulticast Forwarding Cache\n%s%s",
				    " Hash  Origin-Subnet    Mcastgroup   ",
				    "   # pkts In-Vif  Out-Vifs/Forw-ttl\n");
				banner_printed = 1;
			}

			if (kflag) {
				if (kread((u_long)mfcp, &mfc, sizeof(mfc)))
					return;
				mfcp = &mfc;
			}
			printf(" %3u   %-15.15s", i,
		            routename(mfcp->mfc_origin.s_addr));
			printf("  %-15.15s %5s   %2u    ",
			    routename(mfcp->mfc_mcastgrp.s_addr),
			    pktscale(mfcp->mfc_pkt_cnt), mfcp->mfc_parent);
			for (vifi = 0; vifi <= numvifs; ++vifi)
				if (mfcp->mfc_ttls[vifi]) 
					printf(" %u/%u", vifi, 
					    mfcp->mfc_ttls[vifi]);

			printf("\n");
			nmfc++;
		}
	}
	if (!banner_printed)
		printf("\nMulticast Forwarding Cache is empty\n");
	else 
		printf("\nTotal no. of entries in cache: %d\n", nmfc);

	printf("\n");

	if (mfctab != mfctable)
		free(mfctab);
}


static struct mrtstat mrtstat;
static int mrtstat_mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_MRTSTATS };
static struct data_info mrtstat_info = {
    "_mrtstat",
    mrtstat_mib, sizeof(mrtstat_mib)/sizeof(*mrtstat_mib),
    &mrtstat, sizeof(mrtstat)
};

static u_long upcall_data[MAXVIFS];
static struct data_info upcall_info = {
    "_upcall_data",
    NULL, 0,
    &upcall_data, sizeof(upcall_data)
};

void
mrt_stats()
{
	int i, j, k;
	u_long max;

	if (skread("multicast stats", &mrtproto_info) ||
	    skread("multicast stats", &mrtstat_info))
		return;

	switch (mrtproto) {
	case 0:
		warnx("multicast routing not supported by this system");
		return;

	case IGMP_DVMRP:
		break;

	default:
		warnx("multicast routing protocol %u, unknown", mrtproto);
		return;
	}

	printf("multicast routing:\n");

	printf(" %10qu dgram%s w/no route for origin\n",
	    (u_quad_t)mrtstat.mrts_no_route, PLURAL(mrtstat.mrts_no_route));
	printf(" %10qu upcall%s made to mrouted\n",
	    (u_quad_t)mrtstat.mrts_upcalls, PLURAL(mrtstat.mrts_upcalls));
	printf(" %10qu dgram%s w/malformed tunnel options\n",
	    (u_quad_t)mrtstat.mrts_bad_tunnel,
	    PLURAL(mrtstat.mrts_bad_tunnel));
	printf(" %10qu dgram%s w/no room for tunnel options\n",
	    (u_quad_t)mrtstat.mrts_cant_tunnel, 
	    PLURAL(mrtstat.mrts_cant_tunnel));
	printf(" %10qu dgram%s arrived on wrong intf\n",
	    (u_quad_t)mrtstat.mrts_wrong_if, PLURAL(mrtstat.mrts_wrong_if));
	printf(" %10qu dgram%s dropped: upcall Q overflow\n",
	    (u_quad_t)mrtstat.mrts_upq_ovflw, PLURAL(mrtstat.mrts_upq_ovflw));
	printf(" %10qu dgram%s dropped: upcall socket overflow\n",
	    (u_quad_t)mrtstat.mrts_upq_sockfull,
	    PLURAL(mrtstat.mrts_upq_sockfull));
	printf(" %10qu dgram%s cleaned up by the cache\n",
	    (u_quad_t)mrtstat.mrts_cache_cleanups,
	    PLURAL(mrtstat.mrts_cache_cleanups));
	printf(" %10qu dgram%s dropped selectively by ratelimiter\n",
	    (u_quad_t)mrtstat.mrts_drop_sel, PLURAL(mrtstat.mrts_drop_sel));
	printf(" %10qu dgram%s dropped - bucket Q overflow\n",
	    (u_quad_t)mrtstat.mrts_q_overflow,
	    PLURAL(mrtstat.mrts_q_overflow));
	printf(" %10qu dgram%s dropped - larger than bkt size\n",
	    (u_quad_t)mrtstat.mrts_pkt2large, PLURAL(mrtstat.mrts_pkt2large));

	max = 1;
	resolve_quietly = 1;
	if (skread("multicast routing", &upcall_info))
		return;

	for (i = 0; i <= 50; i++)
		max = (max > upcall_data[i]) ? max : upcall_data[i];

	printf("\n\nTiming histogram of upcalls for new packets\n");
	printf("Upcall time(mS)    No. of packets\n");
	
	for (i = 0; i < 50; i++) {
		printf(" %3d - %3d  |", i, i+1);
	    
		if (upcall_data[i] == 0)
	    		j = 0;
		else
	    		j = 1 + 50 * upcall_data[i] / max;

		for (k = 0; k < j; k++)
			printf("%c",'*');

		printf("[%ld]\n", upcall_data[i]);
	}
	printf("  >50       |");

	if (upcall_data[i] == 0)
    		j = 0;
	else
    		j = 1 + 50 * upcall_data[i] / max;
	
	for (k = 0; k < j; k++)
		printf("%c",'*');

	printf(" [%ld]\n", upcall_data[50]);
}

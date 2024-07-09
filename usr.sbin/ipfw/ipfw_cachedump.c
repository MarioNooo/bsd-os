/*-
 *  Copyright (c) 1999 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfw_cachedump.c,v 1.1 2000/01/24 21:52:44 prb Exp
 */
#define	IPFW
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/bpf.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ipfw.h>
#include <netinet/ipfw_cache.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ipfw.h"

void
display_cache(int point, ipfw_filter_t *iftp, int verbose)
{
	ip_cache_hdr_t *h;
	ip_cache_t *c;
	char a[256];

        h = (ip_cache_hdr_t *)((char *)iftp + iftp->hlength);
        c = (ip_cache_t *)(h + 1);

	printf("Address Cache:\n");
	for (;h->ac_entries-- > 0; ++c)
		printf("\t%s\n", inet_ntop(AF_INET, &c->ac_a, a, sizeof(a)));
}

/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfw_throttledump.c,v 1.1 2000/01/24 21:52:46 prb Exp
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/bpf.h>
#include <net/if.h>
#define	IPFW
#include <netinet/in.h>
#include <netinet/ipfw.h>
#include <netinet/ipfw_throttle.h>
#include <arpa/inet.h>

#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
compar(const void *vp1, const void *vp2)
{
        ip_throttle_t *f1 = (ip_throttle_t *)vp1;
        ip_throttle_t *f2 = (ip_throttle_t *)vp2;

        if (f1->th_when < f2->th_when)
                return (1);
        if (f1->th_when == f2->th_when)
                return (0);
        return (-1);
}

extern char dashes[];

void
display_throttle(int point, ipfw_filter_t *iftp, int verbose)
{
	ip_throttle_hdr_t *h;
	ip_throttle_t *t;
	char buf[64];

	char *b;
	clock_t now;

	time(&now);

	b = (char *)iftp + iftp->hlength;
	h = (ip_throttle_hdr_t *)b;
	b = (char *)(h + 1);

	printf("%d packets / %d seconds\n", h->th_limit, h->th_seconds);
	if (h->th_maxentries)
		printf("%d of %d entries used\n", h->th_entries,
		    h->th_maxentries);
	else
		printf("%d entires used\n", h->th_entries);
	if (h->th_mask)
		printf("mask of %08x\n", h->th_mask);

	qsort(b, h->th_entries, sizeof(ip_throttle_hdr_t), compar);
	t = (ip_throttle_t *)b;

	printf("%15s %5s %15s %5s %8s %8s %6s %s\n",
	    "Src Address", "Port",
	    "Dst Address", "Port",
	    "Accepted",
	    "Denied",
	    "Age",
	    "Weight");
	printf("%.15s %.5s %.15s %.5s %.8s %.8s %.6s %.6s\n",
	    dashes, dashes, dashes, dashes, dashes, dashes, dashes, dashes);
	for (; h->th_entries-- >  0; ++t) {
		printf("%15s %5d ",
		    inet_ntop(AF_INET, &t->th_a[0], buf, sizeof(buf)),
		    ntohs(t->th_ports >> 16));
		printf("%15s %5d ",
		    inet_ntop(AF_INET, &t->th_a[1], buf, sizeof(buf)),
		    ntohs(t->th_ports & 0xffff));
		printf("%8qd %8qd ", t->th_accepted, t->th_denied);
		printf("%6ld ", now - t->th_when);
		printf("%d\n", t->th_weight);
	}
}

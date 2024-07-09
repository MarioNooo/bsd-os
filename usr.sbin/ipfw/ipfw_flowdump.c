/*-
 *  Copyright (c) 1999 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfw_flowdump.c,v 1.1 2000/01/24 21:52:45 prb Exp
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
#include <netinet/ipfw_flow.h>
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

int oflag = 0;
static struct timeval now, tv;

static int
compar(const void *vp1, const void *vp2)
{
	ip_flow_t *f1 = (ip_flow_t *)vp1;
	ip_flow_t *f2 = (ip_flow_t *)vp2;

	if (f1->fl_last < f2->fl_last)
		return (1);
	if (f1->fl_last == f2->fl_last)
		return (0);
	return (-1);
}


static char *
pquad(u_quad_t q)
{
	static char buf[64];

	if (q < 10000LL)
		sprintf(buf, "%qd", q);
	else if (q < 10000000LL)
		sprintf(buf, "%qdK", (q + 500LL) / 1000LL);
	else if (q < 10000000000LL)
		sprintf(buf, "%qdM", (q + 500000LL) / 1000000LL);
	else if (q < 10000000000000LL)
		sprintf(buf, "%qdG", (q + 500000000LL) / 1000000000LL);
	else if (q < 10000000000000000LL)
		sprintf(buf, "%qdT", (q + 500000000000LL) / 1000000000000LL);
	else
		sprintf(buf, "%qdP", (q + 500000000000000LL) / 1000000000000000LL);

	return(buf);
}

void
showflow(ipfw_filter_t *filter, ip_flow_t *fl)
{
	char a[64];

	tv.tv_sec = (u_int)(fl->fl_last >> 32);
	tv.tv_usec = (u_int)(fl->fl_last & 0xffffffff);
	tv.tv_sec -= (u_int)(fl->fl_start >> 32);
	tv.tv_usec -= (u_int)(fl->fl_start & 0xffffffff);
	while (tv.tv_usec < 0) {
		tv.tv_sec--;
		tv.tv_usec += 1000000;
	}

	if (oflag) {
		printf("%d ", filter ? filter->serial :
		    ((ip_flow_log_hdr_t *)fl)->fl_serial);
		printf("%d ", fl->fl_protocol);
		printf("%s %d ", inet_ntop(AF_INET, &fl->fl_a[0], a, sizeof(a)),
		    ntohs(fl->fl_ports >> 16));
		printf("%s %d ", inet_ntop(AF_INET, &fl->fl_a[1], a, sizeof(a)),
		    ntohs(fl->fl_ports & 0xffff));
		printf("%lu.%06lu %lu.%06lu %lu.%06lu ",
			(u_long)(fl->fl_start >> 32),
			(u_long)(fl->fl_start & 0xffffffff),
			(u_long)(fl->fl_last >> 32),
			(u_long)(fl->fl_last & 0xffffffff),
			tv.tv_sec, tv.tv_usec);
		printf("%qd %qd %qd %qd\n", fl->fl_bytes[0],
			fl->fl_bytes[1],
			fl->fl_packets[0],
			fl->fl_packets[1]);
		return;
	}
	printf("%2d ", fl->fl_protocol);
	printf("%15s %5d ", inet_ntop(AF_INET, &fl->fl_a[0], a, sizeof(a)),
	    ntohs(fl->fl_ports >> 16));
	printf("%15s %5d ", inet_ntop(AF_INET, &fl->fl_a[1], a, sizeof(a)),
	    ntohs(fl->fl_ports & 0xffff));

	tv.tv_usec += 5000;
	tv.tv_usec /= 10000;
	while (tv.tv_usec >= 100) {
		tv.tv_sec += 1;
		tv.tv_usec -= 100;
	}
	printf("%6ld.%02ld ", tv.tv_sec, tv.tv_usec);

	tv.tv_sec = now.tv_sec - (u_int)(fl->fl_last >> 32);
	tv.tv_usec = now.tv_usec - (u_int)(fl->fl_last & 0xffffffff);
	while (tv.tv_usec < 0) {
		tv.tv_sec--;
		tv.tv_usec += 1000000;
	}
	tv.tv_usec += 5000;
	tv.tv_usec /= 10000;
	while (tv.tv_usec >= 100) {
		tv.tv_sec += 1;
		tv.tv_usec -= 100;
	}
	printf("%6ld.%02ld ", tv.tv_sec, tv.tv_usec);

	printf("%5s", pquad(fl->fl_bytes[0]));
	printf(" %5s", pquad(fl->fl_bytes[1]));
	printf(" [%5s", pquad(fl->fl_packets[0]));
	printf(" %5s]\n", pquad(fl->fl_packets[1]));
}

void
display_flow(int point, ipfw_filter_t *iftp, int verbose)
{
	ip_flow_hdr_t *h;
	ip_flow_t *f;

        h = (ip_flow_hdr_t *)((char *)iftp + iftp->hlength);
        f = (ip_flow_t *)(h + 1);

	gettimeofday(&now, NULL);

	qsort(f, h->flows, sizeof(*f), compar);

	if (oflag == 0) {
		static int second = 0;
		if (second++)
			printf("\n");
		printf(" P ");
		printf("%15s %5s ", "srcaddr", "port");
		printf("%15s %5s ", "dstaddr", "port");
		printf("%9s ", "duration");
		printf("%9s ", "lastuse");
		printf("%5s", "b-in");
		printf(" %5s", "b-out");
		printf(" [%5s", "p-in");
		printf(" %5s]\n", "p-out");
	}
	for (;h->flows-- > 0; ++f)
		showflow(iftp, f);
}

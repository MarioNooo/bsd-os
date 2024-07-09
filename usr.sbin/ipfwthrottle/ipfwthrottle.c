/*-
 *  Copyright (c) 1999 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwthrottle.c,v 1.2 2003/06/06 16:00:19 polk Exp
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
#include <netinet/ipfw_throttle.h>

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

int rcvbuf = 16 * 1024;
int nflag = 0;
int oflag = 0;
int vflag = 0;
int mflag = 0;
struct timeval now, tv;
char dashes[] = "-----------------------";

int compar(const void *vp1, const void *vp2)
{
	ip_throttle_t *f1 = (ip_throttle_t *)vp1;
	ip_throttle_t *f2 = (ip_throttle_t *)vp2;

	if (f1->th_when < f2->th_when)
		return (1);
	if (f1->th_when == f2->th_when)
		return (0);
	return (-1);
}


struct timeval
strtotime(char *res)
{
	struct timeval tv;
	char *ep;
	u_int r, q;

	q = 0;
	while (*res) {
		r = strtol(res, &ep, 0);
		if (!ep || ep == res ||
		    ((r == LONG_MIN || r == LONG_MAX) && errno == ERANGE)) {
invalid:
			errx(1, "invalid time");
		}
		switch (*ep++) {
		case '\0':
			--ep;
			break;
		case 's': case 'S':
			break;
		case 'm': case 'M':
			r *= 60;
			break;
		case 'h': case 'H':
			r *= 60 * 60;
			break;
		case 'd': case 'D':
			r *= 60 * 60 * 24;
			break;
		case 'w': case 'W':
			r *= 60 * 60 * 24 * 7;
			break;
		case 'y': case 'Y':	/* Pretty absurd */
			r *= 60 * 60 * 24 * 365;
			break;
		default:
			goto invalid;
		}
		res = ep;
		q += r;
	}
	
	gettimeofday(&tv, NULL);
	tv.tv_sec -= q;
	tv.tv_usec = 0;
	return (tv);
}


int
main(int argc, char **argv)
{
	struct {
		ipfw_filter_t		ipfw;
		ip_throttle_hdr_t	th;
	} s;
	char a[64], *e;
        ipfw_filter_t *filter;
	ip_throttle_hdr_t *thp;
	ip_throttle_t *th;
	unsigned char *buf;
	int c;
	int n;
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
		IPFW_CALL, IPFWCTL_SYSCTL, IPFW_THROTTLE, /*serial*/0,
		IPFWTHROTTLE_CLOCK };

	if (argc <= 1)
		goto over;
	if (strcmp(argv[1], "input") == 0)
		mib[4] = IPFW_INPUT;
	else if (strcmp(argv[1], "output") == 0)
		mib[4] = IPFW_OUTPUT;
	else if (strcmp(argv[1], "forward") == 0)  
		mib[4] = IPFW_FORWARD;
	else if (strcmp(argv[1], "pre-input") == 0)
		mib[4] = IPFW_PREINPUT;
	else if (strcmp(argv[1], "pre-output") == 0)
		mib[4] = IPFW_PREOUTPUT;
	else if (strcmp(argv[1], "call") == 0)
		mib[4] = IPFW_CALL;
        else
		goto over;
	argv[1] = argv[0];
	++argv;
	--argc;
over:

	memset(&s, 0, sizeof(s));
	s.ipfw.type = IPFW_THROTTLE;
	s.ipfw.hlength = sizeof(s.ipfw);
	s.ipfw.length = sizeof(s.th);
	s.th.th_limit = 30;
	s.th.th_seconds = 60;
	s.th.th_flags = -1;
	tv.tv_sec = tv.tv_usec = 0;

	while ((c = getopt(argc, argv, "Ab:Dm:Ss:T:t:nvx:")) != EOF) {
		switch (c) {
		case 'A':
			s.th.th_flags = 0;
			break;
		case 'b':
			s.th.th_size = strtol(optarg, 0, 0);
			break;
		case 'D':
			if (s.th.th_flags == -1)
				s.th.th_flags = 0;
			s.th.th_flags |= TH_CHKDSTA;
			break;
		case 'x':
			s.th.th_maxentries = strtol(optarg, 0, 0);
			break;
		case 'm':
			s.th.th_mask = strtol(optarg, 0, 0);
			break;
		case 'S':
			if (s.th.th_flags == -1)
				s.th.th_flags = 0;
			s.th.th_flags |= TH_CHKSRCA;
			break;
		case 's':
			mib[7] = s.ipfw.serial = strtol(optarg, 0, 0);
			break;
		case 'T':
			strncpy(s.ipfw.tag, optarg, IPFW_TAGLEN);
			break;
		case 't':
			tv = strtotime(optarg);
			break;
		case 'n':
			nflag++;
			break;
		case 'v':
			vflag++;
			break;
		default: usage:

			fprintf(stderr,
"usage:\n\
ipfwthrottle [display-options]\n\
ipfwthrottle [init-options] packets/seconds\n\
\n\
display-options:\n\
    -t n	Expire entries not seen in last n seconds\n\
    -n		Do not sort the output\n\
    -v		Only display meta information about the flow monitor\n\
    -s serial	Only look at the specified flow monitor\n\
\n\
init-options:\n\
    -D		Only check the destination address\n\
    -S		Only check the source address\n\
    -b buckets	Set the number of hash buckets\n\
    -x max	Never allow more than max entries\n\
    -m mask	Use this mask for the first 32 bits (defaults to 0)\n");
			exit(1);
		}
	}

	if (s.th.th_flags == -1)
		s.th.th_flags = TH_CHKSRCA | TH_CHKDSTA;

	if (optind != argc && optind + 1 != argc)
		goto usage;

	if (optind + 1 == argc) {
		s.th.th_limit = strtol(argv[optind], &e, 0);
		if (e == argv[optind] || *e != '/')
			goto usage;
		argv[optind] = e + 1;
		s.th.th_seconds = strtol(argv[optind], &e, 0);
		if (e == argv[optind] || *e)
			goto usage;

		if (s.ipfw.serial == 0) {
			n = sizeof(s.ipfw.serial);
			mib[5] = IPFWCTL_SERIAL;
			if (sysctl(mib, 6, &s.ipfw.serial, &n, NULL, NULL))
				err(1, "getting serial number");
		}
		mib[5] = IPFWCTL_PUSH;
		if (sysctl(mib, 6, NULL, NULL, &s, sizeof(s)))
			err(1, "pushing throttle filter");
		printf("Installed throttle filter as serial #%d\n",
		    s.ipfw.serial);
		exit (0);
	}

	if (tv.tv_sec) {
		if (sysctl(mib, 9, NULL, NULL, &tv.tv_sec, sizeof(clock_t)) < 0)
			err(1, "Timing out old entries");
	}
	mib[5] = IPFWCTL_PUSH;
	if (sysctl(mib, 8, NULL, &n, NULL, 0) < 0)
		err(1, "getting throttle stats");
	n += sizeof(ip_throttle_t) * 1024;
	if ((buf = malloc(n)) == NULL)
		errx(1, NULL);
	if (sysctl(mib, 8, buf, &n, NULL, 0) < 0)
		err(1, "getting throttle stats");
	gettimeofday(&now, NULL);
	if (n == 0)
		errx(0, "no filters installed");
	if (n < sizeof(*filter) + sizeof(*thp))
		errx(1, "Returned filter truncated");

	while (n > 0) {
		filter = (ipfw_filter_t *)buf;
		n -= (IPFW_next(filter) - buf);
		buf = IPFW_next(filter);

		if (filter->type != IPFW_THROTTLE)
		    continue;

		thp = (ip_throttle_hdr_t *)(filter + 1);
		th = (ip_throttle_t *)(thp + 1);

		n -= sizeof(*filter) + sizeof(*thp);

		printf("Filter #%d\n", filter->serial);
		printf("%d packets / %d seconds\n", thp->th_limit, thp->th_seconds);
		printf("%d entries used\n", thp->th_entries);

		if (vflag)
			continue;

		if (!nflag)
			qsort(th, thp->th_entries, sizeof(*th), compar);

		printf("%15.15s %5.5s %15.15s %5.5s %8.8s %8.8s %6.6s %.6s\n",
		    "Src Address", "Port", "Dst Address", "Port",
		    "Accepted", "Denied", "Age", "Weight");
		printf("%15.15s %5.5s %15.15s %5.5s %8.8s %8.8s %6.6s %.6s\n",
		    dashes, dashes, dashes, dashes,
		    dashes, dashes, dashes, dashes);
		for (;thp->th_entries-- > 0; ++th) {
			printf("%15s %5d ",
			    inet_ntop(AF_INET, &th->th_a[0], a, sizeof(a)),
			    ntohs(th->th_ports >> 16));
			printf("%15s %5d ",
			    inet_ntop(AF_INET, &th->th_a[1], a, sizeof(a)),
			    ntohs(th->th_ports & 0xffff));
			printf("%8qd %8qd ", th->th_accepted, th->th_denied);
			printf("%6ld ", now.tv_sec - th->th_when);
			printf("%d\n", th->th_weight);
		}
	}
	exit (0);
}

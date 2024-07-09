/*-
 *  Copyright (c) 1999 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwechochk.c,v 1.2 2003/06/06 16:00:07 polk Exp
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
#include <netinet/ipfw_echochk.h>

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
	ip_echochk_t *f1 = (ip_echochk_t *)vp1;
	ip_echochk_t *f2 = (ip_echochk_t *)vp2;

	if (f1->ec_when < f2->ec_when)
		return (1);
	if (f1->ec_when == f2->ec_when)
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
		ip_echochk_hdr_t	th;
	} s;
	char a[64], *e;
        ipfw_filter_t *filter;
	ip_echochk_hdr_t *thp;
	ip_echochk_t *th;
	unsigned char *buf;
	int c;
	int n;
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
		IPFW_CALL, IPFWCTL_SYSCTL, IPFW_ECHOCHK, /*serial*/0,
		IPFWECHOCHK_CLOCK };

	memset(&s, 0, sizeof(s));
	s.ipfw.type = IPFW_ECHOCHK;
	s.ipfw.hlength = sizeof(s.ipfw);
	s.ipfw.length = sizeof(s.th);
	tv.tv_sec = tv.tv_usec = 0;
	strncpy(s.ipfw.tag, "echochk", IPFW_TAGLEN);

	while ((c = getopt(argc, argv, "Ab:Dm:Ss:T:t:nvx:")) != EOF) {
		switch (c) {
		case 'b':
			s.th.ec_size = strtol(optarg, 0, 0);
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
ipfwechochk [display-options]\n\
ipfwechochk [init-options] maxentries\n\
\n\
display-options:\n\
    -t n	Expire entries not seen in last n seconds\n\
    -n		Do not sort the output\n\
    -v		Only display meta information about the filter\n\
    -s serial	Only look at the specified flow monitor\n\
\n\
init-options:\n\
    -b buckets	Set the number of hash buckets\n");
			exit(1);
		}
	}

	if (optind != argc && optind + 1 != argc)
		goto usage;

	if (optind + 1 == argc) {
		s.th.ec_maxentries = strtol(argv[optind], &e, 0);
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
			err(1, "pushing echochk filter");
		printf("Installed echochk filter as serial #%d\n",
		    s.ipfw.serial);
		exit (0);
	}

	if (tv.tv_sec) {
		if (sysctl(mib, 9, NULL, NULL, &tv.tv_sec, sizeof(clock_t)) < 0)
			err(1, "Timing out old entries");
	}
	mib[5] = IPFWCTL_PUSH;
	if (sysctl(mib, 8, NULL, &n, NULL, 0) < 0)
		err(1, "getting echochk stats");
	n += sizeof(ip_echochk_t) * 1024;
	if ((buf = malloc(n)) == NULL)
		errx(1, NULL);
	if (sysctl(mib, 8, buf, &n, NULL, 0) < 0)
		err(1, "getting echochk stats");
	gettimeofday(&now, NULL);
	if (n == 0)
		errx(0, "no filters installed");
	if (n < sizeof(*filter) + sizeof(*thp))
		errx(1, "Returned filter truncated");

	while (n > 0) {
		filter = (ipfw_filter_t *)buf;
		n -= (IPFW_next(filter) - buf);
		buf = IPFW_next(filter);

		if (filter->type != IPFW_ECHOCHK)
		    continue;

		thp = (ip_echochk_hdr_t *)(filter + 1);
		th = (ip_echochk_t *)(thp + 1);

		n -= sizeof(*filter) + sizeof(*thp);

		printf("Filter #%d\n", filter->serial);
		printf("%d entries used\n", thp->ec_entries);

		if (vflag)
			continue;

		if (!nflag)
			qsort(th, thp->ec_entries, sizeof(*th), compar);

		printf("%15.15s %15.15s %8.8s %8.8s\n",
		    "Src Address", "Dst Address", "IdSeq", "Age");
		printf("%15.15s %15.15s %8.8s %8.8s\n",
		    dashes, dashes, dashes, dashes);
		for (;thp->ec_entries-- > 0; ++th) {
			printf("%15s ",
			    inet_ntop(AF_INET, &th->ec_a[0], a, sizeof(a)));
			printf("%15s ",
			    inet_ntop(AF_INET, &th->ec_a[1], a, sizeof(a)));
			printf("%08x %8d\n", th->ec_idseq,
			    now.tv_sec - th->ec_when);
		}
	}
	exit (0);
}

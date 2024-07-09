/*-
 *  Copyright (c) 1999 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwflow.c,v 1.3 2003/06/06 16:00:13 polk Exp
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

int rcvbuf = 16 * 1024;
int nflag = 0;
int oflag = 0;
int vflag = 0;
int mflag = 0;
struct timeval now, tv;

int compar(const void *vp1, const void *vp2)
{
	ip_flow_t *f1 = (ip_flow_t *)vp1;
	ip_flow_t *f2 = (ip_flow_t *)vp2;

	if (f1->fl_last < f2->fl_last)
		return (1);
	if (f1->fl_last == f2->fl_last)
		return (0);
	return (-1);
}


char *
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
monitor()
{
	char buf[8192], *b;
	ip_flow_log_hdr_t *flh;
	int r;
	int fd;
	struct sockaddr sa;

	if ((fd = socket(PF_INET, SOCK_RAW, IPFW_FLOW_PROTO)) < 0)
		err(1, "socket(PF_INET, SOCK_RAW, IPFW_FLOW_PROTO)");

#ifdef	SO_MREAD
	r = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_MREAD, &r, sizeof(r)) < 0)
		err(1, "setsockopt(MREAD)");
#endif

	if (rcvbuf && setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf,
	    sizeof(rcvbuf)) < 0)
		err(1, "setsockopt(RCVBUF)");

	memset(&sa, 0, sizeof(sa));
	sa.sa_len = 2;
	sa.sa_family = AF_INET;

	if (bind(fd, &sa, 2))
		err(1, "bind");

	while ((r = read(fd, buf, sizeof(buf))) > 0) {
		if (r < 4)
			continue;
		b = buf;
		flh = (ip_flow_log_hdr_t *)b;
		while (r > 0 && r >= flh->fl_length) {
			b += flh->fl_length;
			r -= flh->fl_length;
			now.tv_sec = flh->fl_expired.tv_sec;
			now.tv_usec = flh->fl_expired.tv_nsec / 1000;
			showflow(0, (ip_flow_t *)flh);
			flh = (ip_flow_log_hdr_t *)b;
		}
	}
}

void
main(int argc, char **argv)
{
	struct {
		ipfw_filter_t	ipfw;
		ip_flow_hdr_t	fh;
	} s;
        ipfw_filter_t *filter;
	ip_flow_hdr_t *fhp;
	ip_flow_t *fl;
	unsigned char *buf;
	int c;
	int n;
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
		IPFW_PREINPUT, IPFWCTL_SYSCTL, IPFW_FLOW, /*serial*/0,
		IPFWFLOW_CLOCK };

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
	s.ipfw.type = IPFW_FLOW;
	s.ipfw.hlength = sizeof(s.ipfw);
	s.ipfw.length = sizeof(s.fh);
	tv.tv_sec = tv.tv_usec = 0;

	while ((c = getopt(argc, argv, "f:F:g:i:ms:t:nov")) != EOF) {
		switch (c) {
		case 'f':
			s.fh.flows = strtol(optarg, 0, 0);
			break;
		case 'F':
			s.fh.maxflows = strtol(optarg, 0, 0);
			break;
		case 'g':
			s.fh.glue = strtol(optarg, 0, 0);
			break;
		case 'i':
			s.fh.index = if_nametoindex(optarg);
			if (s.fh.index <= 0) {
				s.fh.index = strtol(optarg, 0, 0);
				if (s.fh.index <= 0)
					err(1, "%s: no such interface");
			}
			break;
		case 'm':
			mflag++;
			break;
		case 'o':
			oflag++;
			break;
		case 's':
			mib[7] = s.ipfw.serial = strtol(optarg, 0, 0);
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
ipfwflow [filter] [display-options]\n\
ipfwflow [filter] [init-options] {in|out|both} nbuckets\n\
\n\
display-options:\n\
    -m		Monitor logging socket for discarded flows\n\
    -o		Output machine format rather than human format\n\
    -s serial	Only look at the specified flow monitor (no effect with -m)\n\
    -t n	Expire entries not seen in last n seconds\n\
    -n		Do not sort the output\n\
    -v		Only display meta information about the flow monitor\n\
\n\
init-options:\n\
    -f prefill	Put prefill flows on the free list for later consumption\n\
    -F maxflows	Never have more than maxflows flows\n\
    -i index	Only monitor the specified interface\n\
    -g serial	Glue flow monitor to existing flow monitor serial\n\
\n\
where filter is one of: input, forward, output, pre-input, pre-output\n");
			exit(1);
		}
	}

	if (optind != argc && optind + 2 != argc)
		goto usage;

	if (optind + 2 == argc) {
		s.fh.size = strtol(argv[optind+1], 0, 0);
		if (strcmp(argv[optind], "in") == 0)	
			s.fh.direction = 0;
		else if (strcmp(argv[optind], "out") == 0)	
			s.fh.direction = 1;
		else if (strcmp(argv[optind], "both") == 0)	
			s.fh.direction = 2;
		else
			goto usage;

		if (s.ipfw.serial == 0) {
			n = sizeof(s.ipfw.serial);
			mib[5] = IPFWCTL_SERIAL;
			if (sysctl(mib, 6, &s.ipfw.serial, &n, NULL, NULL))
				err(1, "getting serial number");
		}
		mib[5] = IPFWCTL_PUSH;
		if (sysctl(mib, 6, NULL, NULL, &s, sizeof(s)))
			err(1, "pushing flow monitor");
		printf("Installed flow monitoras serial #%d\n", s.ipfw.serial);
		exit (0);
	}
	if (mflag) {
		if (oflag == 0) {
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
		monitor();
		exit (0);
	}
	if (tv.tv_sec || tv.tv_usec) {
		if (sysctl(mib, 9, NULL, NULL, &tv, sizeof(tv)) < 0)
			err(1, "Timing out old entries");
	}
	mib[5] = IPFWCTL_PUSH;
	if (sysctl(mib, 8, NULL, &n, NULL, 0) < 0)
		err(1, "getting flow monitor stats");
	n += sizeof(ip_flow_t) * 1024;
	if ((buf = malloc(n)) == NULL)
		errx(1, NULL);
	if (sysctl(mib, 8, buf, &n, NULL, 0) < 0)
		err(1, "getting flow monitor stats");
	gettimeofday(&now, NULL);
	if (n == 0)
		errx(0, "no filters installed");
	if (n < sizeof(*filter) + sizeof(*fhp))
		errx(1, "Returned filter truncated");

	while (n > 0) {
		filter = (ipfw_filter_t *)buf;
		n -= (IPFW_next(filter) - buf);
		buf = IPFW_next(filter);

		if (filter->type != IPFW_FLOW)
		    continue;

		fhp = (ip_flow_hdr_t *)(filter + 1);
		fl = (ip_flow_t *)(fhp + 1);

		n -= sizeof(*filter) + sizeof(*fhp);

		if (vflag) {
			printf("Filter #%d\n", filter->serial);
			if (fhp->glue)
				printf("filter glued to serial #%d\n",
				    fhp->glue);
			else
				printf("%d flows allocated\n", fhp->flows);
			continue;
		}

		if (!nflag)
			qsort(fl, fhp->flows, sizeof(*fl), compar);

		if (oflag == 0) {
			static int second = 0;
			if (second++)
				printf("\n");
			printf("Filter %d:\n", filter->serial);
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
		for (;fhp->flows-- > 0; ++fl)
			showflow(filter, fl);
	}
	exit (0);
}

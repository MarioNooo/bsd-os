/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwcircuit.c,v 1.7 2001/10/03 17:29:59 polk Exp
 */
#define	IPFW
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <net/bpf.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ipfw.h>
#include <netinet/ipfw_circuit.h>
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

static void tick(int s);
void expire(int serial, int location);
void prune(int serial, u_quad_t when);
void printclk(int serial);

#define	NLOCS	0x80 /* 0x80 */
int nlocs = 0;
int serial;
struct itimerval itv;

int
main(int argc, char **argv)
{
	static int noisey;

        ipfw_filter_t *filter;
	ipfw_cf_hdr_t *circuit;
	ipfw_cf_list_t list[1], *lp;
	unsigned char buf[1024*16+4];
	int chain, index, c, size, which, insert, follow, eflag, max;
	size_t n;
	u_long mask, hit, miss;
	size_t oldlen, *old;
	char *tag = NULL;
	u_quad_t when;

        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW, IPFW_CALL,
		IPFWCTL_SYSCTL, IPFW_CIRCUIT, /*serial*/0, CIRCUITCTL_ADD };

	mask = 0xffffffff;
	size = 0;
	noisey = 0;
	which = 0;
	chain = 0;
	serial = 0;
	index = 0;
	follow = 0;
	insert = -1;
	eflag = 0;
	max = 0;
	when = 0;

	hit = IPFW_ACCEPT;
	miss = IPFW_REJECT;

	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 0;

	if (argc < 1)
		goto usage;

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
		goto skip;

	argv[1] = argv[0];
	++argv;
	--argc;

skip:
	while ((c = getopt(argc, argv, "H:M:N:R:D:Fa:cd:e:f:i:m:n:s:t:T:vW:w:")) != EOF) {
		switch (c) {
		case 'D':
			mib[8] = CIRCUITCTL_BUCKETS;
			serial = strtol(optarg, 0, 0);
			break;
		case 'H':
			hit = strtol(optarg, 0, 0);
			break;
		case 'M':
			miss = strtol(optarg, 0, 0);
			break;
		case 'R':
			mib[8] = 0;
			serial = strtol(optarg, 0, 0);
			break;
		case 'a':
			serial = strtol(optarg, 0, 0);
			break;
		case 'c':
			chain++;
			break;
		case 'd':
			mib[8] = CIRCUITCTL_DELETE;
			serial = strtol(optarg, 0, 0);
			break;
		case 'e':
			serial = strtol(optarg, 0, 0);
			++eflag;
			break;

		case 'F':
			follow = 1;
			break;
		case 'f':
			if (mib[4] != IPFW_CALL)
				errx(1, "-f only available for call filters");
			insert = strtol(optarg, 0, 0);
			break;
		case 'i':
			index = strtol(optarg, 0, 0);
			break;
		case 'm':
			mask = strtol(optarg, 0, 0);
			break;
		case 'n':
			nlocs = strtol(optarg, 0, 0);
			if (nlocs < 1 || nlocs > NLOCS)
				errx(1, "-t takes a value between 1 and %d",
				    NLOCS);
			break;
		case 'N':
			max = strtol(optarg, 0, 0);
			if (max < 1)
				errx(1, "-N takes a positve value");
			break;
		case 's':
			size = strtol(optarg, 0, 0);
			break;
		case 't':
			itv.it_interval.tv_sec = strtol(optarg, 0, 0);
			if (itv.it_interval.tv_sec < 1 ||
			    itv.it_interval.tv_sec > 0xffff)
				errx(1, "-t takes a value between 1 and %d",
				    0xffff);
			break;
		case 'T':
			tag = optarg;
			break;
		case 'v':
			noisey++;
			break;
		case 'W':
			when = strtoll(optarg, 0, 0);
			if (when < 1)
				errx(1, "-W takes a positve value");
			break;

		case 'w':
			if (strcasecmp(optarg, "source") == 0 ||
			    strcasecmp(optarg, "src") == 0)
				which |= CIRCUIT_SRCONLY;
			else if (strcasecmp(optarg, "destination") == 0 ||
			    strcasecmp(optarg, "dst") == 0)
				which |= CIRCUIT_DSTONLY;
			else
				goto usage;
			break;
		default: usage:

			fprintf(stderr,
"ipfwcircuit [filter] [-cv] [-f filter] [-i iface] [-m mask] [-w src|dst]\n"
"	addr1 [addr2] ports\n"
"ipfwcircuit [filter] -a serial [-cv] [-w src|dst] addr1 [addr2] ports [chain]\n"
"ipfwcircuit [filter] -d serial [-cv] [-w src|dst] addr1 [addr2] ports\n"
"ipfwcircuit [filter] -e serial [-n ticks] [-t tickrate]\n"
"\n"
"where filter is one of: input, forward, output, pre-input, pre-output\n"
"      iface is the index number of the interface to be filtered\n");
			exit(1);
		}
	}

	/*
	 * Check to see if we are just adding an entry to an existing filter.
	 * We only add one, but this could add multiple of them
	 */
	if (serial) {
		if (eflag) {
			if (argc > optind)
				goto usage;
			if (when)
				prune(serial, when);
			if (chain)
				printclk(serial);
			if (chain || when)
				exit (0);

			if (itv.it_interval.tv_sec == 0)
				itv.it_interval.tv_sec = 225;
			if (nlocs == 0)
				nlocs = NLOCS;

			signal(SIGALRM, tick);
			itv.it_value = itv.it_interval;
			setitimer(ITIMER_REAL, &itv, NULL);
			for (;;)
				pause();
		}
		mib[7] = serial;

		if (mib[8] == CIRCUITCTL_BUCKETS) {
			if (sysctl(mib, 9, NULL, &oldlen, NULL, 0) < 0)
				err(1, "getting bucket count");
			if ((old = malloc(oldlen)) == NULL)
				err(1, "allocating buckets");
			if (sysctl(mib, 9, old, &oldlen, NULL, 0) < 0)
				err(1, "getting bucket");
			oldlen /= sizeof(size_t);
			for (c = 0; c < oldlen; ++c)
				printf("%d\n", old[c]);
			exit (0);
		}
		if (mib[8] == 0) {
			srandom(getpid());
			if ((oldlen = size) == 0) {
				mib[8] = CIRCUITCTL_BUCKETS;
				if (sysctl(mib, 9, NULL, &oldlen, NULL, 0) < 0)
					err(1, "getting bucket count");
				oldlen *= 2;
			}

			if ((lp = malloc(sizeof(list) * oldlen)) == NULL)
				err(1, "getting list");

			/* 16 random destinations with 16 random ports */
			for (c = 0; c < oldlen; ++c) {
				if (c < oldlen / 8)
					lp[c].cl_a1.s_addr = random();
				else
					lp[c].cl_a1 = lp[random()%(oldlen/8)].cl_a1;
				if (c < 16) {
					lp[c].cl_a2.s_addr = random();
					lp[c].cl_ports = random();
				} else {
					lp[c].cl_a2 = lp[random()&0xf].cl_a2;
					mask = random();
					lp[c].cl_ports = lp[mask&0xf].cl_ports ^
					    (mask & 0xffff0000);
				}
			}
			mib[8] = CIRCUITCTL_ADD;
			if (sysctl(mib, 9, NULL, NULL, lp,
			    sizeof(list) * oldlen) < 0)
				err(1, "sysctl");
			exit (0);
		}

		memset(list, 0, sizeof(list));

		if (argc < optind + 3 + (chain ? 1 : 0) - (which ? 1 : 0))
			goto usage;

		list[0].cl_a1.s_addr = inet_addr(argv[optind++]);
		if (which == 0 || which == 2)
			list[0].cl_a2.s_addr = inet_addr(argv[optind++]);
		list[0].cl_ports = htonl(strtol(argv[optind++], 0, 0));
		list[0].cl_ports &= htonl(mask);
		if (chain)
			list[0].cl_chain = strtol(argv[optind++], 0, 0);

		if (sysctl(mib, 9, NULL, NULL, list,
		    sizeof(ipfw_cf_list_t)) < 0)
			err(1, "sysctl");
		if (noisey) {
			printf("%sFiltering %15s",
			    mib[8] == CIRCUITCTL_DELETE ? "No longer " : "",
			    inet_ntoa(list[0].cl_a1));
			printf(" -> %15s", inet_ntoa(list[0].cl_a2));
			printf(" %5d -> %5d\n",
			    ntohl(list[0].cl_ports) >> 16,
			    ntohl(list[0].cl_ports) & 0xffff);
		}
		exit(0);
	}
	if (eflag)
		goto usage;
	if (optind < argc)
		size = strtol(argv[optind++], 0, 0);
	if (optind != argc)
		goto usage;

	/*
	 * Generate an empty circuit table entry
	 */
	filter = (ipfw_filter_t *)buf;
	circuit = (ipfw_cf_hdr_t *)(filter + 1);

	memset(filter, 0, sizeof(*filter));
	memset(circuit, 0, sizeof(*circuit));

	n = sizeof(serial);
	mib[5] = IPFWCTL_SERIAL;
	if (sysctl(mib, 6, &serial, &n, NULL, NULL))
		err(1, "getting serial number");

	filter->type = IPFW_CIRCUIT;
	filter->length = sizeof(*circuit);
	filter->serial = serial;
	if (tag)
		strncpy(filter->tag, tag, IPFW_TAGLEN);
	circuit->size = size <= 0 ? 997 : size;
	circuit->hitcode = IPFW_ACCEPT;
	circuit->misscode = IPFW_REJECT;
	if (mib[4] != IPFW_CALL)
		circuit->misscode |= IPFW_NEXT;
	circuit->datamask = htonl(mask);
	circuit->protocol = IPPROTO_TCP;
	circuit->which = which;
	circuit->index = index;
	circuit->flip16 = which ? 0 : 1;
	circuit->chain = chain ? 1 : 0;
	circuit->follow = follow ? 1 : 0;

	circuit->slices = itv.it_interval.tv_sec;
	circuit->timeout = itv.it_interval.tv_sec * nlocs;
	circuit->maxcircuits = max;

	mib[5] = IPFWCTL_PUSH;
	if (sysctl(mib, 6, NULL, NULL, buf, (u_char *)(circuit + 1) - buf))
		err(1, "pushing circuit");

	if (noisey)
		printf("Installed as serial #%d with mask %08lx\n", serial, mask);
	if (insert >= 0) {
		mib[5] = IPFWCTL_INSERT;
		mib[6] = insert;
		if (sysctl(mib, 7, NULL, NULL, &serial, sizeof(serial)))
			err(1, "insert circuit");

		printf("Inserted filter #%d in location %d\n", serial, insert);
	}
	exit (0);
}

static void
tick(int s)
{
	static int location = 0;
	expire(serial, location);
	location = (location + 1) & (nlocs-1);
}

void
expire(int serial, int location)
{
	static u_quad_t lastclock[NLOCS] = { 0 };
	size_t size;
	int i;

#define	LASTCLOCK	lastclock[(location+1) & (nlocs-1)]
#define	THISCLOCK	lastclock[location & (nlocs-1)]

        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW, IPFW_CALL,
		IPFWCTL_SYSCTL, IPFW_CIRCUIT, /*serial*/0, CIRCUITCTL_CLOCK };

	mib[7] = serial;
	size = sizeof(u_quad_t);
	if (LASTCLOCK)
		i = sysctl(mib, 9, &THISCLOCK, &size, &LASTCLOCK, sizeof(u_quad_t));
	else
		i = sysctl(mib, 9, &THISCLOCK, &size, NULL, 0);
	if (i < 0)
		warn("circuit clock %d, tick %d", serial, location);
}

void
prune(int serial, u_quad_t when)
{
	int i;

        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW, IPFW_CALL,
		IPFWCTL_SYSCTL, IPFW_CIRCUIT, /*serial*/0, CIRCUITCTL_CLOCK };

	mib[7] = serial;
	i = sysctl(mib, 9, NULL, NULL, &when, sizeof(u_quad_t));
	if (i < 0)
		warn("circuit expire %d @ %qd", serial, when);
}

void
printclk(int serial)
{
	int i;
	size_t size;
	u_quad_t when;

        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW, IPFW_CALL,
		IPFWCTL_SYSCTL, IPFW_CIRCUIT, /*serial*/0, CIRCUITCTL_CLOCK };

	mib[7] = serial;

	size = sizeof(when);
	i = sysctl(mib, 9, &when, &size, NULL, 0);
	if (i == 0)
		printf("%qd\n", when);
}

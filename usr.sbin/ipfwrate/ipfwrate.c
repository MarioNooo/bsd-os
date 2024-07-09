/*-
 *  Copyright (c) 1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwrate.c,v 1.4 1999/08/29 02:14:53 prb Exp
 */
#define	IPFW
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ipfw.h>
#include <netinet/ip_rate.h>
#include <netinet/ip_rateband.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	B2b(x) 	((quad_t)(x) << 3)
#define	b2B(x) 	((x) >> 3)

int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_RATEFILTER, 0, 0 };

#define	INDEX	mib[4]
#define	CMD	mib[5]
#define	LENGTH	(sizeof(mib)/sizeof(mib[0]))

void printstats(struct iprq_ratestats *);

void
main(int ac, char **av)
{
	struct iprq_bandwidth_params p;
	char sbuf[sizeof(struct iprq_bandwidth_stats)];
	struct iprq_ratestats *s;
	quad_t q;
	size_t n;
	int c, type;
	int needinit;

	memset(&p, 0, sizeof(p));
	p.rbwp_qlen = 16;
	needinit = 0;

	while ((c = getopt(ac, av, "iq:p:Q:M:")) != EOF)
	switch (c) {
	case 'i':
		++needinit;
		break;

	case 'p':
		q = strtoq(optarg, 0, 0);
		if (q <= 8 || q >= (quad_t)8 << 32)
			fprintf(stderr, "%s: out of range for pbps\n", optarg);
		p.rbwp_pbps = q >> 3;    /* We keep track of bytes, not bits */
		break;

	case 'q':
		q = strtoq(optarg, 0, 0);
		if (q <= 8 || q >= (quad_t)4 << 32)
			fprintf(stderr, "%s: out of range for quant\n", optarg);
		p.rbwp_quant = q >> 3;    /* We keep track of bytes, not bits */
		break;

	case 'Q':
		q = strtoq(optarg, 0, 0);
		if (q <= 0 || q > 0xffff)
			fprintf(stderr, "%s: out of range for qlen\n", optarg);
		p.rbwp_qlen = q;
		break;

	case 'M':
		q = strtoq(optarg, 0, 0);
		if (q <= 0 || q > 0xffff)
			fprintf(stderr, "%s: out of range for mqlen\n", optarg);
		p.rbwp_mqlen = q;
		break;

	}


	switch (ac - optind) {
#if 0
	case 3:
		INDEX = atoi(av[1]);
		i = atoi(av[2]);
		if (i <= 0)
			i = 1;
		CMD = IRCTL_TYPE;

		n = sizeof(type);
		if (sysctl(mib, LENGTH, &type, &n, NULL, 0) < 0)
			err(1, "retrieving filter type for %d", INDEX);
		if (type != IRTYPE_BANDWIDTH)
			errx(1, "type %d is not IRTYPE_BANDWIDTH\n", type);
		n = sizeof(os);

		CMD = IRCTL_STATS;
		if (sysctl(mib, LENGTH, &os, &n, NULL, 0) < 0)
			err(1, "retrieving filter %d", INDEX);
		for (;;) {
			tv.tv_usec = 0;
			tv.tv_sec = i;
			select(0, 0, 0, 0, &tv);
			n = sizeof(sbuf);
			if (sysctl(mib, LENGTH, &sbuf, &n, NULL, 0) < 0)
				err(1, "retrieving filter %d", INDEX);
			os.iprs_passthru = s.iprs_passthru - os.iprs_passthru;
			os.iprs_relayed = s.iprs_relayed - os.iprs_relayed;
			os.iprs_relayed += os.iprs_passthru;
			os.iprs_relayed -= s.iprs_bls;
			os.iprs_relayed += os.iprs_bls;
			os.iprs_relayed <<= 3;
			os.iprs_last.tv_sec = s.iprs_last.tv_sec - os.iprs_last.tv_sec;
			os.iprs_last.tv_usec = s.iprs_last.tv_usec - os.iprs_last.tv_usec;
			d = oiba.iprs_last.tv_sec;
			d += (double)oiba.iprs_last.tv_usec / 1000000.0;
			if (d)
				oiba.iprs_relayed = (quad_t)((double)oiba.iprs_relayed / d);
			else
				oiba.iprs_relayed = 0;
			printf("%8qd (%7qd)\n", oiba.iprs_relayed, oiba.iprs_relayed/8);
			os = s;
		}
		break;
#endif

	case 1:
		CMD = IRCTL_STATS;
		INDEX = atoi(av[optind]);
		if (INDEX <= 0)
			errx(1, "%s: invalid index", av[optind]);
		n = sizeof(sbuf);
		if (sysctl(mib, LENGTH, &sbuf, &n, NULL, 0) < 0)
			err(1, "retrieving filter %d", INDEX);
		s = (struct iprq_ratestats *)sbuf;
		if (s->iprs_index != INDEX)
			warnx("reported index %d is not %d", s->iprs_index, INDEX);
		printstats(s);
		break;

	case 2:
		CMD = IRCTL_STATS;
		INDEX = atoi(av[optind]);
		if (INDEX <= 0)
			errx(1, "%s: invalid index", av[optind]);

		if (needinit == 0) {
			n = sizeof(sbuf);
			if (sysctl(mib, LENGTH, &sbuf, &n, NULL, 0) < 0)
				needinit = 1;
			else {
				s = (struct iprq_ratestats *)sbuf;
				if (s->iprs_type != IRTYPE_BANDWIDTH)
					needinit = 1;
			}
		}

		if (needinit) {
			CMD = IRCTL_TYPE;
			type = IRTYPE_BANDWIDTH;

			if (sysctl(mib, LENGTH, NULL, NULL, &type,
			    sizeof(type)) < 0)
				err(1, "setting filter type for %d", INDEX);
		}

		CMD = IRCTL_PARAMS;

		q = strtoq(av[optind+1], 0, 0);
		if (q <= 8 || q >= (quad_t)8 << 32)
			fprintf(stderr, "%s: out of range for bps\n",
			    av[optind+1]);
		p.rbwp_bps = q >> 3;	/* We keep track of bytes, not bits */
		if (sysctl(mib, LENGTH, NULL, NULL, &p, sizeof(p)) < 0)
			err(1, "setting filter %d", INDEX);
		break;

	case 0:
		INDEX = 0;
		CMD = IRCTL_STATS;
		if (sysctl(mib, LENGTH, NULL, &n, NULL, 0) < 0)
			err(1, "getting size of filter stats");
		if ((s = malloc(n)) == NULL)
			err(1, NULL);
		
		if (sysctl(mib, LENGTH, s, &n, NULL, 0) < 0)
			err(1, "getting filter stats");

		while (n > 0) {
			printstats(s);
			if ((n -= s->iprs_length) > 0)
				printf("\n");
			s = (struct iprq_ratestats *)((char *)s + s->iprs_length);
		}
		break;

	default:
		fprintf(stderr, "usage: ipfwrate [-i] [-p pbps] [-q quant] [-Q qlen] [-M mqlen] [index [bps]]\n");
		exit(1);
	}
	exit(0);
}

void
printstats(struct iprq_ratestats *s)
{
	struct iprq_bandwidth_stats *bs;
	struct iprq_bandwidth_params p;
	size_t m;

	printf("%-16s: %d\n", "Filter Index", s->iprs_index);
	switch (s->iprs_type) {
	case IRTYPE_BANDWIDTH:
		printf("%-16s: %s\n", "Filter Type", "bandwidth");
		INDEX = s->iprs_index;
		CMD = IRCTL_PARAMS;
		m = sizeof(p);
		if (sysctl(mib, LENGTH, &p, &m, NULL, 0) < 0)
			errx(1, "retrieving filter %d parameters",
			    INDEX);

		bs = (struct iprq_bandwidth_stats *)s;
		printf("%-16s: %qd bps\n", "Filter Rate", B2b(p.rbwp_bps));
		printf("%-16s: %qd bps\n", "Promised Rate", B2b(p.rbwp_pbps));
		printf("%-16s: %qd bits\n", "Bits in Que", B2b(bs->rbws_bls));
		printf("%-16s: %qd bits\n", "Bits in PQue", B2b(bs->rbws_pbls));
		printf("%-16s: %qd bits\n", "Quantize", B2b(p.rbwp_quant));
		printf("%-16s: %ld packets\n", "Max IF Que Len", p.rbwp_mqlen);
		break;

	default:
		printf("%-16s: %d\n", "Filter Type", s->iprs_type);
		break;
	}
	printf("%-16s: %d packets\n", "Que Length", s->iprs_qlen);
	printf("%-16s: %d packets\n", "In Queue", s->iprs_qleft);
	printf("%-16s: %qd bytes\n", "passthru", s->iprs_passthru);
	printf("%-16s: %qd bytes\n", "delayed", s->iprs_delayed);
	printf("%-16s: %qd bytes\n", "relayed", s->iprs_relayed);
	printf("%-16s: %qd bytes\n", "dropped", s->iprs_dropped);
	/* printf("%-16s: %qd bytes\n", "ifdropped", s->iprs_ifdropped); */
}

/*-
 *  Copyright (c) 2000 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwcache.c,v 1.2 2002/01/31 04:13:50 prb Exp
 */
#define	IPFW
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
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
#include <netdb.h>

#include "ipfw.h"

int ipfw_tagtoserial(int which, char *tag);

int
main(int ac, char **av)
{
	static int noisey;
	ip_cache_t ce;
	char buf[256], *b;
	struct {
		ipfw_filter_t	filter;
		ip_cache_hdr_t	cache;
	} s;
	int c;

        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW, IPFW_CALL,
		IPFWCTL_SYSCTL, IPFW_CACHE, /*serial*/0, IPFWAC_ADD };

	memset(&s, 0, sizeof(s));

	while ((c = getopt(ac, av, "ds:T:v")) != EOF) {
		switch (c) {
		case 'd':
			mib[8] = IPFWAC_DELETE;
			break;

		case 's':
			s.cache.ac_size = strtol(optarg, 0, 0);
			break;

		case 'T':
			strncpy(s.filter.tag, optarg, IPFW_TAGLEN);
			break;

		case 'v':
			noisey++;
			break;

		default:
			fprintf(stderr,
"ipfwcache [-dv] [-s size] [-T tag] [address [...]\n");
			exit(1);
		}
	}

	if (optind < ac) {
		if (s.filter.tag[0] == '\0')
			errx(1, "Must specify tag with -T to add/delete entries");
		mib[7] = ipfw_tagtoserial(IPFW_CALL, s.filter.tag);
		if (mib[7] < 0)
			errx(1, "%s: invalid tag", s.filter.tag);
		if (mib[7] == 0)
			errx(1, "%s: tag not found", s.filter.tag);

		while (optind < ac) {
			struct addrinfo h, *ai, *pai;
			char *nm;
			int e;
			u_long mask, cnt;
			struct in_addr last_addr;

			memset(&h, 0, sizeof(h));
			h.ai_family = PF_INET;

			if ((nm = strchr(av[optind], '/')) != NULL) {
				mask = strtoul(nm + 1, &b, 0);
				if (mask < 0 || mask > 32)
					errx(1, "%s: invalid netmask", av[optind]);
				if (mask < 16)
					errx(1, "%s: netmasks but be at least 16 bits", av[optind]);
				*nm = '\0';
				mask = 0xffffffff << (32 - mask);
			} else
				mask = 0xffffffff;
			if ((e = getaddrinfo(av[optind], NULL, &h, &ai)) != 0)
				errx(1, "%s: %s", av[optind], gai_strerror(e));

			if (mask != 0xffffffff && ai->ai_next) {
				*nm = '/';
				warnx("%s: resolves to too more than a single host name, netmask not allowed", av[optind]);
			}

#define	ADDR	((struct sockaddr_in *)pai->ai_addr)

			last_addr.s_addr = 0xffffffff;
			for (pai = ai; pai; pai = pai->ai_next) {
				cnt = mask;
				if (ADDR->sin_addr.s_addr == last_addr.s_addr)
					continue;
				last_addr = ADDR->sin_addr;
				NTOHL(ADDR->sin_addr.s_addr);
				ce.ac_a.s_addr = ADDR->sin_addr.s_addr & mask;
				HTONL(ce.ac_a.s_addr);
				do {
					if (sysctl(mib, 9, NULL, NULL, &ce,
					    sizeof(ce)) < 0) {
						if (mib[8] != IPFWAC_DELETE)
						    err(1, "adding %s",
							av[optind]);
					} else if (noisey) {
					    b = (char *)
						inet_ntop(pai->ai_family,
						&ce.ac_a, buf,
						sizeof(buf));
					    if (b)
						printf("%s %s\n",
						    mib[8] == IPFWAC_DELETE ?
						    "Deleted" : "Added", b);
					}
					NTOHL(ce.ac_a.s_addr);
					ce.ac_a.s_addr++;
					HTONL(ce.ac_a.s_addr);
					++cnt;
				} while (cnt != 0);
			}

			freeaddrinfo(ai);
			++optind;
		}
		exit (0);
	}

	/*
	 * Generate an empty cache table
	 */
	s.filter.type = IPFW_CACHE;
	s.filter.hlength = sizeof(s.filter);
	s.filter.length = sizeof(s.cache);

	mib[5] = IPFWCTL_PUSH;
	if (sysctl(mib, 6, NULL, NULL, &s, sizeof(s)))
		err(1, "pushing cache");

	exit (0);
}

int
ipfw_tagtoserial(int which, char *tag)
{
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW, 0, IPFWCTL_LIST };
	size_t n;
	char *b;
	ipfw_filter_t *iftp;

	mib[4] = which;

	if (strlen(tag) > IPFW_TAGLEN)
		return (-1);

	if (sysctl(mib, 6, NULL, &n, NULL, 0) < 0)
		err(1, "getting list of filters");

	if (n == 0)
		return (0);
	
	if ((b = malloc(n)) == NULL)
		err(1, NULL);

	if (sysctl(mib, 6, b, &n, NULL, 0) < 0)
		err(1, "getting list of filters");

	n /= sizeof(ipfw_filter_t);

	for (iftp = (ipfw_filter_t *)b; n > 0; ++iftp, --n)
		if (strcmp(iftp->tag, tag) == 0)
			return (iftp->serial);
	return (0);
}

/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfw_natdump.c,v 1.7 2000/01/24 21:52:45 prb Exp
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
#include <netinet/ipfw_nat.h>
#include <arpa/inet.h>

#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *
protoname(int p)
{
	static char proto[8];
	switch (p) {
	case IPPROTO_TCP: return ("tcp");
	case IPPROTO_UDP: return ("udp");
	case IPPROTO_ICMP: return ("icmp");
	default:
		sprintf(proto, "%d", p);
		return (proto);
	}
}

int
masktocidr(u_long x)
{
	int i = 32;
	if (x == 0)
		return (0);
	while ((x & 1) == 0) {
		x >>= 1;
		--i;
	}
	return (i);
}

static int
compar(const void *vp1, const void *vp2)
{
        ip_nat_t *f1 = (ip_nat_t *)vp1;
        ip_nat_t *f2 = (ip_nat_t *)vp2;

        if (f1->nat_when < f2->nat_when)
                return (1);
        if (f1->nat_when == f2->nat_when)
                return (0);
        return (-1);
}


void
display_nat(int point, ipfw_filter_t *iftp, int verbose)
{
	char *nb;
	size_t nn;
	char *b;
	ip_nat_hdr_t *h;
	ip_nat_t *nat;
	int i;
	u_long a;
	clock_t now;

	ip_natdef_t *nd;
	ip_natent_t *ne;
	ip_natservice_t *ns;
	ip_natmap_t *nm;

	int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
		point, IPFWCTL_SYSCTL, IPFW_NAT, /*serial*/0,
		IPFWNAT_SETUP };

	b = (char *)iftp + iftp->hlength;
	h = (ip_nat_hdr_t *)b;
	b = (char *)(h + 1);

	if (h->glue)
		printf("Glued to serial %d\n", h->glue);

	mib[7] = iftp->serial;
	if (sysctl(mib, 9, NULL, &nn, NULL, NULL) < 0)
		err(1, "extracting size of nat");
	if ((nb = malloc(nn)) == NULL)
		err(1, NULL);
	time(&now);
	if (sysctl(mib, 9, nb, &nn, NULL, NULL) < 0)
		err(1, "extracting nat");
	nd = (ip_natdef_t *)nb;
	nb += sizeof(ip_natdef_t);
	nn -= sizeof(ip_natdef_t);
	if (nn < 0)
		errx(1, "trunacated nat");
	if (iftp->priority)
		printf("priority %d;\n", iftp->priority);
	if (iftp->tag[0])
		printf("tag \"%.*s\";\n", IPFW_TAGLEN, iftp->tag);
	if (h->index) {
		char ifname[IFNAMSIZ];

		if (if_indextoname(h->index, ifname) == NULL)
			snprintf(ifname, sizeof(ifname), "%d", h->index);
		printf("interface %s;\n", ifname);
	}
	if (h->maxsessions)
		printf("maxsessions %d;\n", h->maxsessions);
	if (h->size != NAT_HASH_SIZE)
		printf("buckets %d;\n", h->size);
	for (i = 0; i < 256; ++i) {
			
		if (h->timeouts[i] !=
		    ((i == IPPROTO_TCP) ? NAT_TCPTIMEOUT : NAT_TIMEOUT))
			printf("timeout %s %d;\n", protoname(i),h->timeouts[i]);
	}

	while (nd->nd_nservices-- > 0) {
		ns = (ip_natservice_t *)nb;
		nb += sizeof(ip_natservice_t);
		nn -= sizeof(ip_natservice_t);
		if (nn < 0)
			errx(1, "truncated service");
		printf("service [%d]", ns->ns_serial);
		if (ns->ns_protocol)
		    printf(" %s", protoname(ns->ns_protocol));
		printf(" %s", inet_ntoa(ns->ns_external));
		if (ns->ns_eport)
			printf(":%d", ntohs(ns->ns_eport));
		printf(" -> %s", inet_ntoa(ns->ns_internal));
		if (ns->ns_iport)
			printf(":%d", ntohs(ns->ns_iport));
		if (ns->ns_expire)
			printf(" expire in %ld", ns->ns_expire - now);
		printf(";\n");
	}
	while (nd->nd_nmaps-- > 0) {
		char *sep = "";
		int indent;
		nm = (ip_natmap_t *)nb;
		nb += sizeof(ip_natmap_t);
		nn -= sizeof(ip_natmap_t);
		if (nn < 0)
			errx(1, "truncated map");
		indent = printf("map [%d]", nm->nm_serial);
		if (nm->nm_protocol)
			indent += printf(" %s", protoname(nm->nm_protocol));
		indent += printf(" %s", inet_ntoa(nm->nm_internal));
		a = masktocidr(ntohl(nm->nm_mask.s_addr));
		if (a && a < 32)
			indent += printf("/%ld", a);
		indent += printf(" -> ");
		while (nm->nm_nentries-- > 0) {
			ne = (ip_natent_t *)nb;
			nb += sizeof(ip_natent_t);
			nn -= sizeof(ip_natent_t);
			if (nn < 0)
				errx(1, "truncated entry");
			printf(sep, indent, "");
			printf("%s", inet_ntoa(ne->ne_firstaddr));

			a = ntohl(ne->ne_lastaddr.s_addr) - ntohl(ne->ne_firstaddr.s_addr);
			if (a == 0 || ne->ne_firstaddr.s_addr == 0)
				;
			else if ((a & (a-1)) == 0)
				printf("/%d", masktocidr(~(a-1)));
			else
				printf("-%s", inet_ntoa(ne->ne_lastaddr));

			if (ntohs(ne->ne_firstport)) {
				printf(" %d", ntohs(ne->ne_firstport));
				printf(" - %d", ntohs(ne->ne_lastport));
			}
			sep = ",\n%*s";
		}
		printf(";\n");
	}
	if (nn > 0)
		warnx("had %d extra bytes", nn);

	if (h->sessions <= 0)
		return;

	printf("\n  %5s %15s:%5s %15s:%5s %15s:%5s",
		"proto",
		"remote", "port",
		"external", "port",
		"internal", "port");
	if (verbose)
		printf(" %7s %7s %8s/%8s %s",
		    "age", "timeout", "ihash", "ohash", "buckets");
	printf("\n");

	qsort(b, h->sessions, sizeof(ip_nat_t), compar);

	while (h->sessions-- >  0) {
		nat = (ip_nat_t *)b;
		b += sizeof(ip_nat_t);


		if (nat->nat_direction == IPFW_PREINPUT >> 24)
			printf("->");
	    	else if (nat->nat_direction == IPFW_PREOUTPUT >> 24)
			printf("<-");
		else
			printf("%02x", nat->nat_direction);
		printf(" %4s", protoname(nat->nat_protocol));
			
		printf(" %15s:%5d", inet_ntoa(nat->nat_remote), ntohs(nat->nat_iprp.p.dst));
		printf(" %15s:%5d", inet_ntoa(nat->nat_external), ntohs(nat->nat_eprp.p.src));
		printf(" %15s:%5d", inet_ntoa(nat->nat_internal), ntohs(nat->nat_iprp.p.src));
		if (verbose) {
			printf(" %7ld", (now - nat->nat_when));
			printf(" %7ld", h->timeouts[nat->nat_protocol] - (now - nat->nat_when));
			printf(" %08lx/%08lx", nat->nat_ihash, nat->nat_ohash);
			printf(" %ld/%ld", nat->nat_ihash % h->size, nat->nat_ohash % h->size);
		}
		printf("\n");
	}
}

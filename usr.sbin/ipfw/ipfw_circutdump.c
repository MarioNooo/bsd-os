/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfw_circutdump.c,v 1.3 2000/01/24 21:52:45 prb Exp
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/bpf.h>
#define	IPFW
#include <netinet/in.h>
#include <netinet/ipfw.h>
#include <netinet/ipfw_circuit.h>
#include <arpa/inet.h>


#include <stdio.h>

void
display_circuitcache(ipfw_filter_t *iftp)
{
	char *b;
	ipfw_cf_hdr_t *h;
	ipfw_cf_list_t *lp;

	b = (char *)iftp + iftp->hlength;

	h = (ipfw_cf_hdr_t *)b;
	b = (char *)(h + 1);

	printf("%sCircuit cache[%d]:",
	    h->protocol == IPPROTO_TCP ? "TCP " :
	    h->protocol == IPPROTO_UDP ? "UDP " : "",
	    h->size); 

	if (h->protocol != IPPROTO_TCP &&
	    h->protocol != IPPROTO_UDP)
		printf(" proto %d", h->protocol);

	if (h->protocol == IPPROTO_TCP && h->follow)
		printf(" [follow acks]");

	if (h->which == CIRCUIT_SRCONLY)
		printf(" src-only");
	else if (h->which == CIRCUIT_DSTONLY)
		printf(" dst-only");

	if (h->datamask == 0)
		printf(" addr-only");
	else if ((h->datamask & 0xffff0000) == 0)
		printf(" src-port");
	else if ((h->datamask & 0x0000ffff) == 0)
		printf(" dst-port");
	printf("\n");

	if (h->index)
		printf("Only filter interface: %d\n", h->index);

	while (h->length-- >  0) {
		lp = (ipfw_cf_list_t *)b;
		b += h->cfsize;
		
		NTOHL(lp->cl_ports);
		printf("%15s ", inet_ntoa(lp->cl_a1));
		if (h->which == 0 || h->which == 2)
			printf("-> %15s ", inet_ntoa(lp->cl_a2));
		if (h->datamask & 0xffff0000)
			printf("%5d", lp->cl_ports >> 16);
		if ((h->datamask & 0xffff0000) &&
		    h->datamask & 0x0000ffff)
			printf(" -> ");
		if (h->datamask & 0x0000ffff)
			printf("%5d", lp->cl_ports & 0xffff);
		printf(" [%qd]\n", lp->cl_when);
	}
}

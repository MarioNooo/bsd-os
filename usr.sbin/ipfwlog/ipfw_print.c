/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfw_print.c,v 1.4 2001/10/03 17:29:59 polk Exp
 */
#define	IPFW
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ipfw.h>
#include <arpa/inet.h>

#include <time.h>
#include <stdio.h>
#include <unistd.h>

/*
 * Print IPFW control messages.  It is bogus that the message starts
 * with an IPv4 header with IP version 0.
 */
void
ipfw_print(unsigned char *b, int n)
{
	struct sockaddr_in sin;
	struct ip *ip;
	int i, s;
	u_short srcport, dstport;
	char host[1025];
	char port[32];
	extern int nflag;

	if (n < sizeof(*ip)) {
		for (i = 0; i < n; ++i)
			log(" %02x", b[i]);
		n = 0;
		return;
	}
	ip = (struct ip *)b;
	n -= sizeof(*ip);
	b += sizeof(*ip);

	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(struct sockaddr_in);

	if (nflag) {
		log(" %15s -> ", inet_ntoa(ip->ip_src));
		log("%15s ", inet_ntoa(ip->ip_dst));
	} else {
		sin.sin_addr = ip->ip_src;
		if (getnameinfo(&sin, sizeof(sin), host, sizeof(host), NULL, 0, 0) == 0)
			log(" %15s -> ", host);
		else
			log(" %15s -> ", inet_ntoa(ip->ip_src));

		sin.sin_addr = ip->ip_dst;
		if (getnameinfo(&sin, sizeof(sin), host, sizeof(host), NULL, 0, 0) == 0)
			log("%15s ", host);
		else
			log("%15s ", inet_ntoa(ip->ip_src));
	}

	if (n < sizeof(u_short) * 2)
		return;

	srcport = (b[0] << 8) | b[1];
	dstport = (b[2] << 8) | b[3];


	log("TCP ");
	if (nflag) {
		log("%5d -> %5d ", srcport, dstport);
	} else {
		sin.sin_port = htons(srcport);
		if (getnameinfo(&sin, sizeof(sin), NULL, 0, port, sizeof(port), 0) == 0)
			log(" %5s -> ", port);
		else
			log(" %5d -> ", srcport);
		sin.sin_port = htons(dstport);
		if (getnameinfo(&sin, sizeof(sin), NULL, 0, port, sizeof(port), 0) == 0)
			log("%5s ", port);
		else
			log("%5d ", dstport);
	}

	log("circuit closed");
}

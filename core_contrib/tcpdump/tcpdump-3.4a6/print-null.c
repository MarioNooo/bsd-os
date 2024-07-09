/*
 * This file has been slightly modified by NRL for use with IPv6+IPsec.
 * Search for INET6 and/or IPSEC to see the blocks where this happened.
 * See the NRL Copyright notice for conditions on the modifications.
 */
/*
 * Copyright (c) 1991, 1993, 1994, 1995, 1996, 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) /master/core_contrib/tcpdump/tcpdump-3.4a6/print-null.c,v 1.4 1998/02/27 05:00:25 jch Exp (LBL)";
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#if __STDC__
struct mbuf;
struct rtentry;
#endif
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>

#include <pcap.h>
#include <stdio.h>
#include <string.h>

#include "interface.h"
#include "addrtoname.h"
#include "extract.h"

#ifndef AF_NS
#define AF_NS		6		/* XEROX NS protocols */
#endif

/*
 * The DLT_NULL packet header is 4 bytes long. It contains a network
 * order 32 bit integer that specifies the family, e.g. AF_INET
 */
#define	NULL_HDRLEN 4

static void
null_print(u_int family, const u_char *p, u_int length)
{

	if (nflag) {
		printf("%d: ", family);
		return;
	}
	switch (family) {

	case AF_INET:
		printf("ip: ");
		break;

	case AF_INET6:
		printf("ipv6: ");
		break;

	case AF_NS:
		printf("ns: ");
		break;

	default:
		printf("AF %d: ", family);
		break;
	}
}

void
null_if_print(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{
	u_int length = h->len;
	u_int caplen = h->caplen;
	u_int family;

	ts_print(&h->ts);

	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	packetp = p;
	snapend = p + caplen;

	family = ntohl(EXTRACT_32BITS(p));

	length -= NULL_HDRLEN;
	p += NULL_HDRLEN;

	if (eflag)
		null_print(family, p, length);

	switch (family) {
	case AF_INET:
		ip_print(p, length);
		break;

#ifdef	INET6
	case AF_INET6:
		ipv6_print(p, length);
		break;
#endif

	case AF_NS:
		ipx_print(p, length);
		break;
	}

	if (xflag)
		default_print(p, caplen - NULL_HDRLEN);
	if (xxflag)
		hexdump(packetp, h->caplen);
	putchar('\n');
}


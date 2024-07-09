/*
 * This file has been slightly modified by NRL for use with IPv6+IPsec.
 * Search for INET6 and/or IPSEC to see the blocks where this happened.
 * See the NRL Copyright notice for conditions on the modifications.
 */
/*
 * Copyright (c) 1989, 1990, 1991, 1993, 1994, 1995, 1996, 1997
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
    "@(#) /master/core_contrib/tcpdump/tcpdump-3.4a6/print-sl.c,v 1.3 1998/02/17 14:47:38 jch Exp (LBL)";
#endif

#ifdef HAVE_NET_SLIP_H
#include <sys/param.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/socket.h>

#if __STDC__
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

#include <net/slcompress.h>
#include <net/slip.h>

#include <ctype.h>
#include <netdb.h>
#include <pcap.h>
#include <stdio.h>

#include "interface.h"
#include "addrtoname.h"
#include "extract.h"			/* must come after interface.h */

static u_int lastlen[2][256];
static u_int lastconn = 255;

static void sliplink_print(const u_char *, const struct ip *, u_int, u_int);

/* BSD/OS 2.1 and later compatibility */
#if !defined(SLIP_HDRLEN) && defined(SLC_BPFHDR)
/* 
 * Each packet has a BPF header of SLC_BPFHDR (24) bytes
 *
 * The format is:
 *	1 Byte Direction			(SLIPDIR_IN or SLIPDIR_OUT)
 *	1 Byte Length of link level header	(0 for slip)
 *	1 Byte Length of compressed header	(0 for slip)
 *	m Bytes link level header 		(maximum of 4 bytes)
 *	n Bytes compressed header 		(maximum of 15 bytes)
 *	p Bytes to pad to SLC_BPFHDR bytes
 */
#define SLIP_HDRLEN	SLC_BPFHDR
#define SLX_DIR		SLC_DIR
#define SLX_CHDR	SLC_CHDR
#endif

void
sl_if_print(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{
	register u_int caplen = h->caplen;
	register u_int length = h->len;
	register const struct ip *ip;

	ts_print(&h->ts);

	if (caplen < SLIP_HDRLEN) {
		printf("[|slip]");
		goto out;
	}
	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	packetp = p;
	snapend = p + caplen;

	length -= SLIP_HDRLEN;

	ip = (struct ip *)(p + SLIP_HDRLEN);

	if (eflag)
		sliplink_print(p, ip, length, h->len);

	if (ip->ip_v == 4)
		ip_print((u_char *)ip, length);
#ifdef	INET6
	else if (ip->ip_v == 6)
		ipv6_print((u_char *)ip, length);
#endif

	if (xflag)
		default_print((u_char *)ip, snapend - p);
 out:
	if (xxflag)
		hexdump((u_char *)ip, snapend - p);
	putchar('\n');
}

static void
sliplink_print(register const u_char *p, register const struct ip *ip,
	       register u_int length, register u_int iplength)
{
	int type;
	int dir;
	u_int hlen;
	int chlen;
	int cid;
	const u_char *chp;

	dir = p[SLX_DIR];
	putchar(dir == SLIPDIR_IN ? 'I' : 'O');
	putchar(' ');

#ifdef	SLC_BPFHDR
	chlen = p[SLC_CHL] & 0x0f;
	chp = p + SLC_BPFHDRLEN + p[SLC_LLHL];
	switch (chlen) {
	case 0:
		type = (ip)->ip_v << 4;
		cid = ip->ip_p;
		chp = 0;
		break;
	case 2:
		chlen = 0;
		cid = chp[1];
		/* Fall through */
	default:
		type = *chp & 0xf0;
		break;
	}
#else
	type = p[SLX_CHDR] & 0xf0;
	chp = p + SLX_CHDR;
	chlen = CHDR_LEN;
	if (chlen)
		cid = ((struct ip *)&ch)->ip_p;
#endif

	switch (type) {

	case TYPE_IP:
		printf("ip %d: ", iplength);
		break;

	case TYPE_UNCOMPRESSED_TCP:
		uncompressed_sl_print(cid, ip, length, dir);
		break;

	default:
		if (type & TYPE_COMPRESSED_TCP)
			compressed_sl_print(chp, ip, length, dir);
		else
			printf("slip-%d!: ", type);
		break;
	}
}

static const u_char *
print_sl_change(const char *str, register const u_char *cp)
{
	register u_int i;

	if ((i = *cp++) == 0) {
		i = EXTRACT_16BITS(cp);
		cp += 2;
	}
	printf(" %s%d", str, i);
	return (cp);
}

static const u_char *
print_sl_winchange(register const u_char *cp)
{
	register short i;

	if ((i = *cp++) == 0) {
		i = EXTRACT_16BITS(cp);
		cp += 2;
	}
	if (i >= 0)
		printf(" W+%d", i);
	else
		printf(" W%d", i);
	return (cp);
}

void
compressed_sl_print(const u_char *chdr, const struct ip *ip,
		    u_int length, int dir)
{
	register const u_char *cp = chdr;
	register u_int flags, hlen;

	flags = *cp++;
	if (flags & NEW_C) {
		lastconn = *cp++;
		printf("ctcp %d", lastconn);
	} else
		printf("ctcp *");

	/* skip tcp checksum */
	cp += 2;

	switch (flags & SPECIALS_MASK) {
	case SPECIAL_I:
		printf(" *SA+%d", lastlen[dir][lastconn]);
		break;

	case SPECIAL_D:
		printf(" *S+%d", lastlen[dir][lastconn]);
		break;

	default:
		if (flags & NEW_U)
			cp = print_sl_change("U=", cp);
		if (flags & NEW_W)
			cp = print_sl_winchange(cp);
		if (flags & NEW_A)
			cp = print_sl_change("A+", cp);
		if (flags & NEW_S)
			cp = print_sl_change("S+", cp);
		break;
	}
	if (flags & NEW_I)
		cp = print_sl_change("I+", cp);

	/*
	 * 'hlen' is the length of the uncompressed TCP/IP header (in words).
	 * 'cp - chdr' is the length of the compressed header.
	 * 'length - hlen' is the amount of data in the packet.
	 */
	hlen = ip->ip_hl;
	hlen += ((struct tcphdr *)&((int32_t *)ip)[hlen])->th_off;
	lastlen[dir][lastconn] = length - (hlen << 2);
	printf(" %d (%d):", lastlen[dir][lastconn], cp - chdr);
}

void
uncompressed_sl_print(u_char cid, const struct ip *ip,
		      u_int length, int dir)
{
	int hlen;

	/*
	 * The connection id is stored in the IP protocol field.
	 * Get it from the link layer since sl_uncompress_tcp()
	 * has restored the IP header copy to IPPROTO_TCP.
	 */

	lastconn = cid;
	hlen = ip->ip_hl;
	hlen += ((struct tcphdr *)&((int *)ip)[hlen])->th_off;
	lastlen[dir][lastconn] = length - (hlen << 2);
	printf("utcp %d: ", lastconn);
}
#else
#include <sys/types.h>
#include <sys/time.h>

#include <pcap.h>
#include <stdio.h>

#include "interface.h"

void
sl_if_print(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{

	error("not configured for slip");
	/* NOTREACHED */
}

void
sl_bsdos_if_print(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{

	error("not configured for slip");
	/* NOTREACHED */
}
#endif

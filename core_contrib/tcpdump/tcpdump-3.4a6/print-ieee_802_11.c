/*
 * This file has been slightly modified by NRL for use with IPv6+IPsec.
 * Search for INET6 and/or IPSEC to see the blocks where this happened.
 * See the NRL Copyright notice for conditions on the modifications.
 */
/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997
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
    "@(#) /master/core_contrib/tcpdump/tcpdump-3.4a6/print-ieee_802_11.c,v 1.4 2000/08/31 13:37:02 polk Exp (LBL)";
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>

#if __STDC__
struct mbuf;
struct rtentry;
#endif
#include <net/if.h>
#include <net/if_802_11.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>

#include <stdio.h>
#include <pcap.h>

#include "interface.h"
#include "addrtoname.h"
#include "ethertype.h"

int ieee_802_11_encap_print(u_short, const u_char *, u_int, u_int);

const u_char *packetp;
const u_char *snapend;

static char *mgt_names[] = {
	"asreq",
	"asresp",
	"reasreq",
	"reasresp",
	"probereq",
	"proberesp",
	"type-0x6",
	"type-0x7",
	"beacon",
	"atim",
	"disas",
	"auth",
	"deauth",
	"type-0xd",
	"type-0xe",
	"type-0xf",
};

static char *ctl_names[] = {
	"type-0x0",
	"type-0x1",
	"type-0x2",
	"type-0x3",
	"type-0x4",
	"type-0x5",
	"type-0x6",
	"type-0x7",
	"type-0x8",
	"type-0x9",
	"ps",
	"rts",
	"cts",
	"ack",
	"end",
	"ack",
};

static char *data_names[] = {
	"data",
	"ack",
	"poll",
	"poll",
	"null",
	"ack",
	"poll",
	"poll",
	"type-0x8",
	"type-0x9",
	"type-0xa",
	"type-0xb",
	"type-0xc",
	"type-0xd",
	"type-0xe",
	"type-0xf",
};

static void
ieee_802_11_print(register const u_char *bp, u_int length)
{
	struct ieee_802_11_header *ep;
	u_short etype;
	int st;

	ep = (struct ieee_802_11_header *)bp;

	printf("%s %s %s %s",
	     etheraddr_string(ep->e11_addr1),
	     etheraddr_string(ep->e11_addr2),
	     etheraddr_string(ep->e11_addr3),
	     etheraddr_string(ep->e11_addr4));

	if (ep->e11_frame_ctl0 & 3)
		printf(" version %x",  ep->e11_frame_ctl0 & 3);

	st = (ep->e11_frame_ctl0 >> 4) & 0xf;
	switch (ep->e11_frame_ctl0 & IEEE_802_11_TYPE_MASK) {
	case IEEE_802_11_TYPE_MGMT:
		printf(" mgtm %s", mgt_names[st]);
		break;

	case IEEE_802_11_TYPE_CTL:
		printf(" ctl %s", ctl_names[st]);
		break;

	case IEEE_802_11_TYPE_DATA:
		printf(" data %s", data_names[st]);
		break;

	case IEEE_802_11_TYPE_RESERVED:
		printf(" reserved type-0x%x", st);
		break;
	}

	if (ep->e11_frame_ctl1 & IEEE_802_11_FCTL_TODS)
		printf(" tods");
	if (ep->e11_frame_ctl1 & IEEE_802_11_FCTL_FROMDS)
		printf(" fromds");
	if (ep->e11_frame_ctl1 & IEEE_802_11_FCTL_MOREFRAGS)
		printf(" morefrags");
	if (ep->e11_frame_ctl1 & IEEE_802_11_FCTL_RETRY)
		printf(" retry");
	if (ep->e11_frame_ctl1 & IEEE_802_11_FCTL_PM)
		printf(" pm");
	if (ep->e11_frame_ctl1 & IEEE_802_11_FCTL_MOREDATA)
		printf(" moredata");
	if (ep->e11_frame_ctl1 & IEEE_802_11_FCTL_WEP)
		printf(" wep");
	if (ep->e11_frame_ctl1 & IEEE_802_11_FCTL_ORDER)
		printf(" order");
	printf(" duration %04x", ntohs(ep->e11_duration));
	printf(" seqctl %04x", ntohs(ep->e11_seq_ctl));

	bp += IEEE_802_11_HDR_LEN;

        if (bp[0] == IEEE_802_11_SNAP_K1 && bp[1] == IEEE_802_11_SNAP_K1 &&
            bp[2] == IEEE_802_11_SNAP_CONTROL && bp[3] == 0)
		printf(" snap %s", etherproto_string(*(u_short *)(bp+6)));
	else
		printf(" %s", etherproto_string(*(u_short *)(bp)));
	printf(" (%d) ", length);
}

/*
 * This is the top level routine of the printer.  'p' is the points
 * to the ether header of the packet, 'tvp' is the timestamp,
 * 'length' is the length of the packet off the wire, and 'caplen'
 * is the number of bytes actually captured.
 */
void
ieee_802_11_if_print(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{
	u_int caplen = h->caplen;
	u_int length = h->len;
	struct ieee_802_11_header *ep;
	u_short ether_type;
	extern u_short extracted_ethertype;

	ts_print(&h->ts);

	if (caplen < IEEE_802_11_HDR_LEN + 8) {
		printf("[|802.11]");
		goto out;
	}

	if (eflag)
		ieee_802_11_print(p, length);

	/*
	 * Some printers want to get back at the ethernet addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	packetp = p;
	snapend = p + caplen;

	length -= IEEE_802_11_HDR_LEN;
	caplen -= IEEE_802_11_HDR_LEN;
	ep = (struct ieee_802_11_header *)p;
	p += IEEE_802_11_HDR_LEN;

	if ((ep->e11_frame_ctl0 & IEEE_802_11_TYPE_MASK) == 
	    IEEE_802_11_TYPE_DATA) {
		if (p[0] == AC_SNAP_K1 && p[1] == AC_SNAP_K1 &&
		    p[2] == AC_SNAP_CONTROL && p[3] == AC_SNAP_K2) {
			length -= 6;
			caplen -= 6;
			p += 6;
		}
		ether_type = ntohs(*(u_short *)p);
		length -= 2;
		caplen -= 2;
		p += 2;
	} else
		ether_type = 0;

        if (ieee_802_11_encap_print(ether_type, p, length, caplen) == 0) {
                /* ether_type not known, print raw packet */
                if (!eflag)
                        ieee_802_11_print((u_char *)ep, length + sizeof(*ep));
                if (!xflag && !qflag && !xxflag)
                        default_print(p, caplen);
        }
	if (xflag)
		default_print(p, caplen);
 out:
	if (xxflag)
		hexdump(packetp, h->caplen);
	putchar('\n');
}

/*
 * Prints the packet encapsulated in an Ethernet data segment
 * (or an equivalent encapsulation), given the Ethernet type code.
 *
 * Returns non-zero if it can do so, zero if the ethertype is unknown.
 *
 * Stuffs the ether type into a global for the benefit of lower layers
 * that might want to know what it is.
 */

u_short	extracted_ethertype;

int
ieee_802_11_encap_print(u_short ethertype, const u_char *p,
    u_int length, u_int caplen)
{
	extracted_ethertype = ethertype;

	switch (ethertype) {

	case ETHERTYPE_IP:
		ip_print(p, length);
		return (1);

#ifdef	INET6
	case ETHERTYPE_IPV6:
		ipv6_print(p, length);
		return (1);
#endif

	case ETHERTYPE_ARP:
	case ETHERTYPE_REVARP:
		arp_print(p, length, caplen);
		return (1);

	case ETHERTYPE_DN:
		decnet_print(p, length, caplen);
		return (1);

	case ETHERTYPE_ATALK:
		if (vflag)
			fputs("et1 ", stdout);
		atalk_print(p, length);
		return (1);

	case ETHERTYPE_AARP:
		aarp_print(p, length);
		return (1);

	case ETHERTYPE_LAT:
	case ETHERTYPE_SCA:
	case ETHERTYPE_MOPRC:
	case ETHERTYPE_MOPDL:
		/* default_print for now */
	default:
		return (0);
	}
}

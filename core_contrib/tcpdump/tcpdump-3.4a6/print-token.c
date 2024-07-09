/*
 * Copyright (c) 1991, 1992, 1993, 1994
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
static  char rcsid[] =
	"@(#)/master/core_contrib/tcpdump/tcpdump-3.4a6/print-token.c,v 1.3 1999/09/19 23:08:22 polk Exp (LBL)";
#endif

#ifdef HAVE_TOKEN
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if_token.h>

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <pcap.h>
#include <signal.h>
#include <stdio.h>

#include "interface.h"
#include "addrtoname.h"
#include "ethertype.h"

/*
 * Token Ring support for tcpdump
 * based on FDDI support for tcpdump, by Jeffrey Mogul [DECWRL], June 1992
 *
 * Based in part on code by Van Jacobson, which bears this note:
 *
 * NOTE:  This is a very preliminary hack for FDDI support.
 * There are all sorts of wired in constants & nothing (yet)
 * to print SMT packets as anything other than hex dumps.
 * Most of the necessary changes are waiting on my redoing
 * the "header" that a kernel fddi driver supplies to bpf:  I
 * want it to look like one byte of 'direction' (0 or 1
 * depending on whether the packet was inbound or outbound),
 * two bytes of system/driver dependent data (anything an
 * implementor thinks would be useful to filter on and/or
 * save per-packet, then the real 21-byte FDDI header.
 * Steve McCanne & I have also talked about adding the
 * 'direction' byte to all bpf headers (e.g., in the two
 * bytes of padding on an ethernet header).  It's not clear
 * we could do this in a backwards compatible way & we hate
 * the idea of an incompatible bpf change.  Discussions are
 * proceeding.
 *
 * Also, to really support FDDI (and better support 802.2
 * over ethernet) we really need to re-think the rather simple
 * minded assumptions about fixed length & fixed format link
 * level headers made in gencode.c.  One day...
 *
 *  - vj
 */


/*
 * Print Token Ring access-control bits
 */
static inline void
print_token_ac(u_char ac)
{
	if (ac & ACF_TOKEN) {
		printf("frame");
	} else {
		printf("token");
	}
	if (ac & 0x08)
		printf(" mac-ctl");
	if (ac & 0xe0)
		printf(" pri %x", (ac & 0xe0) >> 5);
	if (ac & 0x07)
		printf(" res %x", ac & 0x07);
}

/*
 * Print Token Ring frame-control bits
 */
static inline void
print_token_fc(u_char fc)
{
	switch ((fc>>6)&0x03) {
	case 0:
		printf("mac");
		break;
	case 1:
		/* Non-mac frame */
		printf("llc");
		return;
	default:
		printf("type=%d", (fc>>6)&0x03);
	}
	switch (fc&0x0f) {
	case 1:
		printf(" express");
		break;

	case 2:
		printf(" beacon");
		break;

	case 3:
		printf(" claim");
		break;

	case 4:
		printf(" ring-purge");
		break;

	case 5:
		/* Active monitor present */
		printf(" amp");
		break;

	case 6:
		/* Standby monitor present */
		printf(" smp");
		break;

	case 0:
		break;

	default:
		printf(" pcf=%x", fc&0x0f);
	}
}

static void
token_print_rif(const struct token_max_hdr *macp)
{
	int riflen = ROUTE_BYTES(macp);
	u_char *p;
	char *s;
	int route;

	printf("[srcrt ");
	switch (macp->rif.rcf0 & RCF0_BCAST_MASK) {
	case RCF0_ALL_BROADCAST:
		printf("bcast-all ");
		break;
	case RCF0_SINGLE_BROADCAST|RCF0_ALL_BROADCAST:
		printf("bcast-single ");
		break;
	case 0:
		break;
	default:
		printf("bcast-%x ", macp->rif.rcf0 & RCF0_BCAST_MASK);
		break;
	}
	if (vflag) {
		printf("dir=%d ", macp->rif.rcf1 >> 7);
		switch (macp->rif.rcf1 & RCF1_FRAME_MASK) {
		case RCF1_FRAME0:
			route = RCF1_FRAME0_MAX;
			break;
		case RCF1_FRAME1:
			route = RCF1_FRAME1_MAX;
			break;
		case RCF1_FRAME2:
			route = RCF1_FRAME2_MAX;
			break;
		case RCF1_FRAME3:
			route = RCF1_FRAME3_MAX;
			break;
		case RCF1_FRAME4:
			route = RCF1_FRAME4_MAX;
			break;
		case RCF1_FRAME5:
			route = RCF1_FRAME5_MAX;
			break;
		case RCF1_FRAME6:
			route = RCF1_FRAME6_MAX;
			break;
		case 0x70:
			route = -1;
			break;
		}
		if (route > 0) {
			printf("mtu=%d", route);
		} else {
			printf("mtu=***");
		}
		p = (u_char *)macp->rif.rseg;
		s = " ";
		for (riflen -= 2; riflen > 0; riflen -= 2, p += 2) {
			route = *p << 8 | *(p + 1);
			printf("%s%4.4x", s, route);
			s = ":";
		}
	} else {
		printf("len=%d", riflen);
	}
	printf("] ");
}

/* Extract src, dst addresses */
static inline void
extract_token_addrs(const struct token_max_hdr *macp, char *fsrc, char *fdst)
{
	bcopy(macp->hdr.token_dhost, fdst, ETHER_ADDR_LEN);
	bcopy(macp->hdr.token_shost, fsrc, ETHER_ADDR_LEN);
}

/*
 * Print the IEE802.5 MAC header
 */
static inline int
token_print(register const struct token_max_hdr *macp, register int length,
	   register const u_char *fsrc, register const u_char *fdst,
	   int caplen)
{
	char *srcname, *dstname;
	int riflen = 0;

	srcname = etheraddr_string(fsrc);
	dstname = etheraddr_string(fdst);
	if (HAS_ROUTE(macp))
		riflen = ROUTE_BYTES(macp);

	if (vflag) {
		printf("%s %s ", srcname, dstname);
		printf("ac=%02x(", macp->hdr.token_acf);
		print_token_ac(macp->hdr.token_acf);
		printf(") fc=%02x(", macp->hdr.token_fcf);
		print_token_fc(macp->hdr.token_fcf);
		printf(") ");
		if (riflen) {
			if (caplen < TOKEN_MAC_SIZE(macp)) {
				printf("[|token route]");
				return (0);
			}
			token_print_rif(macp);
		}
		printf("%d: ", length);
	}
	else if (qflag)
		printf("%s %s %d: ", srcname, dstname, length);
	else {
		printf("%s %s ", srcname, dstname);
		print_token_ac(macp->hdr.token_acf);
		printf(" ");
		print_token_fc(macp->hdr.token_fcf);
		printf(" ");
		if (riflen) {
			if (caplen < TOKEN_MAC_SIZE(macp)) {
				(void) printf("%d ", length);
				printf("[|token route]");
				return (0);
			}
			token_print_rif(macp);
		}
		printf("%d: ", length);
	}

	return(1);
}

/*
 * This is the top level routine of the printer.  'sp' is the points
 * to the IEE802.5 header of the packet, 'tvp' is the timestamp,
 * 'length' is the length of the packet off the wire, and 'caplen'
 * is the number of bytes actually captured.
 */
void
token_if_print(u_char *pcap, const struct pcap_pkthdr *h,
	      register const u_char *p)
{
	int caplen = h->caplen;
	int length = h->len;
	int riflen = 0;
	const struct token_max_hdr *macp = (struct token_max_hdr *)p;
	extern u_short extracted_ethertype;
	struct ether_header ehdr;

	ts_print(&h->ts);

	if (caplen < sizeof(struct token_header)) {
		printf("[|token]");
		goto out;
	}
	/*
	 * Get the token ring addresses into a canonical form
	 */
	extract_token_addrs(macp, (char*)ESRC(&ehdr), (char*)EDST(&ehdr));
	if (HAS_ROUTE(macp)) {
		riflen = ROUTE_BYTES(macp);
		ehdr.ether_shost[0] &= ~RI_PRESENT;
	}
	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	snapend = p + caplen;
	/*
	 * Actually, the only printer that uses packetp is print-bootp.c,
	 * and it assumes that packetp points to an Ethernet header.  The
	 * right thing to do is to fix print-bootp.c to know which link
	 * type is in use when it excavates. XXX
	 */
	packetp = (u_char *)&ehdr;

	if (eflag &&
	    token_print(macp, length, ESRC(&ehdr), EDST(&ehdr), caplen) == 0)
		goto out;
	if (caplen < sizeof(struct token_header) + riflen) {
		printf("[|token route]");
		goto out;
	}

	/* Skip over IEE802.5  MAC header */
	length -= sizeof(struct token_header) + riflen;
	p += sizeof(struct token_header) + riflen;
	caplen -= sizeof(struct token_header) + riflen;

	/* Frame Control field determines interpretation of packet */
	if (macp->hdr.token_fcf & 0xc0) {
		extracted_ethertype = 0;
		/* Try to print the LLC-layer header & higher layers */
		if (llc_print(p, length, caplen, ESRC(&ehdr), 
	  	    EDST(&ehdr)) == 0) {
			/*
			 * Some kinds of LLC packet we cannot
			 * handle intelligently
			 */
			if (!eflag)
				(void) token_print(macp, length, ESRC(&ehdr),
				    EDST(&ehdr), caplen);
			if (extracted_ethertype) {
				printf("(LLC %s) ",
				 etherproto_string(htons(extracted_ethertype)));
			}
			if (!xflag && !qflag && !xxflag)
				default_print(p, caplen);
		}
	} else {
		/* Jump dump MAC data */
		printf("MAC control");
		if (!xflag && !qflag && !xxflag) 
			default_print(p, caplen);
	}
	if (xflag)
		default_print(p, caplen);
out:
	if (xxflag)
		hexdump(macp, h->caplen);
	putchar('\n');
}
#else
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>

#include "interface.h"
void
token_if_print(u_char *pcap, const struct pcap_pkthdr *h, const u_char *p)
{

	error("not configured for token ring");
	/* NOTREACHED */
}
#endif

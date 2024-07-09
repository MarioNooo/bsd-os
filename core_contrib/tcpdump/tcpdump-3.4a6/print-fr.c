/*
 * Copyright (c) 1991, 1993, 1994
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
static char rcsid[] =
    "@(#)/master/core_contrib/tcpdump/tcpdump-3.4a6/print-fr.c,v 1.2 2000/02/18 22:22:50 jch Exp (LBL)";
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>

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


#include <stdio.h>

#include "extract.h"
#include "interface.h"
#include "addrtoname.h"
#include "pcap.h"

#define	FR_HDRLEN 4

#define FR_EA1          0x0100
#define FR_EA2          0x0001
#define FR_CR           0x0200
#define FR_DE           0x0002
#define FR_BECN         0x0004
#define FR_FECN         0x0008
#define FR_GET_DLCI(x)  ((((x) & 0xf0) >> 4) | ((x) & 0xfc00) >> 6)
#define FR_SET_DLCI(x)  ((((x) & 0x0f) << 4) | ((x) & 0x3f0) << 6)

#define FR_UI           0x03
#define FR_PAD          0x00
#define FR_LMI_FRIF_PROTO       0x09    /* FRIF */
#define FR_LMI_ANSI_PROTO       0x08    /* ANSI/CCITT */
#define FR_LMI_CCITT_PROTO      0x08    /* ANSI/CCITT */
#define FR_NLPID_SNAP           0x80    /* IARP packet */
#define FR_NLPID_IP             0xcc    /* IP packet */

#define	FR_CISCO_IP	0x8000	/* Cisco IP */
#define	FR_CISCO_CDP	0x2000	/* Cisco Discovery Protocol */

fr_dlci(const u_char *p)
{
	u_short fh;

	fh = (p[0] << 8) | p[1];
	return (FR_GET_DLCI(fh));
}

static void
fr_print(const u_char *p, const struct ip *ip, int length)
{
	u_long dlci;
	u_short fh;

	fh = (p[0] << 8) | p[1];
	if ((fh & FR_EA1) || (fh & FR_EA2) == 0) {
		printf("[%02x:%02x:%02x:%02x] ", p[0], p[1], p[2], p[3]);
		return;
	}
	dlci = FR_GET_DLCI(fh);
	printf("DLCI-%d-%c ", dlci, (fh & FR_CR ? 'C' : 'R'));
	if (fh & FR_DE)
		printf("D ");
	if (fh & FR_BECN)
		printf("BECN ");
	if (fh & FR_FECN)
		printf("FECN ");
	switch (p[2]) {
	case FR_UI:
		break;
	default:
		printf("Frame-%02x ", p[2]);
		break;
	}
	if (p[3] == '\0') {
		printf("PAD ");
		++p;
	}
	switch (p[3]) {
	case FR_LMI_FRIF_PROTO:
		printf("FRIF-LMI: ");
		break;
	case FR_LMI_ANSI_PROTO:
		printf("LMI: ");
		break;
	case FR_NLPID_SNAP:
		printf("IARP: ");
		break;
	case FR_NLPID_IP:
		printf("ip: ");
		break;
	default:
		printf("proto-%02x: ", p[3]);
		break;
	}
}

static void
lmi_print(const u_char *p, int length, u_long dlci)
{
	printf("lmi ");
	if (length > 0) {
	    while (length-- > 1)
		    printf("%02x.", *p++);
	    printf("%02x.", *p);
	}
}

static void
snap_print(const u_char *p, int length, u_long dlci)
{
	printf("snap-%d ", dlci);
	if (length > 0) {
	    while (length-- > 1)
		    printf("%02x.", *p++);
	    printf("%02x.", *p);
	}
}

static void
unk_print(const u_char *p, int length, u_long dlci)
{
	if (length > 0) {
	    while (length-- > 1)
		    printf("%02x.", *p++);
	    printf("%02x.", *p);
	}
}

static void
cdp_print(const u_char *p, int length, u_long dlci)
{
	printf("cdp-%d ", dlci);
	/* XXX - This calls code in print-chdlc.c */
	chdlc_cdp_print(p, length);
}

void
fr_if_print(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{
	int length = h->len;
	int caplen = h->caplen;
	const struct ip *ip;
	int hlen;

	ts_print(&h->ts);

	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	packetp = p;
	snapend = p + caplen;

	hlen = FR_HDRLEN;

	if (p[3] != FR_PAD)
		--hlen;
		
	length -= hlen - 1;

	ip = (struct ip *)(p + hlen + 1);

	if (eflag)
		fr_print(p, ip, length);

	if (p[hlen - 1] == FR_UI)
		switch (p[hlen]) {
		case FR_LMI_FRIF_PROTO:
		case FR_LMI_ANSI_PROTO:
			lmi_print((const u_char *)ip, caplen - FR_HDRLEN,
			    fr_dlci(p));
			break;
		case FR_NLPID_SNAP:
			snap_print((const u_char *)ip, caplen - FR_HDRLEN,
			    fr_dlci(p));
			break;
		case FR_NLPID_IP:
			ip_print((const u_char *)ip, length);
			if (xflag)
				default_print((const u_char *)ip,
				    caplen - FR_HDRLEN);
			break;
		default:
			printf("[|%02x] ", p[hlen]);
			unk_print((const u_char *)ip, caplen - FR_HDRLEN,
			    fr_dlci(p));
			break;
		}
	else {
		u_int proto = EXTRACT_16BITS(p + hlen - 1);

		switch (proto) {
		case FR_CISCO_IP:
			ip_print((const u_char *)ip, length);
			if (xflag)
				default_print((const u_char *)ip,
				    caplen - FR_HDRLEN);
			break;

		case FR_CISCO_CDP:
			cdp_print((const u_char *)ip, length, fr_dlci(p));
			break;

		default:
			printf("[|%04x] ", proto);
			unk_print((const u_char *)ip, caplen - FR_HDRLEN,
			    fr_dlci(p));
			break;
		}
	}

	if (xxflag)
		hexdump(packetp, h->caplen);
	putchar('\n');
}

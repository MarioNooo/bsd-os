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
    "@(#) /master/core_contrib/tcpdump/tcpdump-3.4a6/print-arp.c,v 1.4 2001/04/26 01:17:46 jch Exp (LBL)";
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>

#if __STDC__
struct mbuf;
struct rtentry;
#endif
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdio.h>
#include <string.h>

#include "interface.h"
#include "addrtoname.h"
#include "ethertype.h"
#include "extract.h"			/* must come after interface.h */

/* Compatibility */
#ifndef REVARP_REQUEST
#define REVARP_REQUEST		3
#endif
#ifndef REVARP_REPLY
#define REVARP_REPLY		4
#endif
#ifndef	INARP_REQUEST
#define	INARP_REQUEST		8
#endif
#ifndef	INARP_REPLY
#define	INARP_REPLY		9
#endif
#ifndef	ARP_NAK
#define	ARP_NAK			10
#endif
#ifndef	ARPHDR_ATM
#define	ARPHDR_ATM		19
#endif

struct	atmarphdr {
	u_short	ar_hrd;		/* format of hardware address */
	u_short	ar_pro;		/* format of protocol address */
	u_char	ar_shtl;	/* Type & length of source ATM number (q) */
	u_char	ar_sstl;	/* Type & length of source ATM subaddress (r) */
	u_short	ar_op;		/* Operation code (request, reply, or NAK) */
	u_char	ar_spln;	/* Length of source protocol address (s) */
	u_char	ar_thtl;	/* Type & length of target ATM number (x) */
	u_char	ar_tstl;	/* Type & length of target ATM subaddress (y) */
	u_char	ar_tpln;	/* Length of target protocol address (z) */
#ifdef	COMMENT_ONLY
	u_char	ar_sha[];	/* qoctets  source ATM number */
	u_char	ar_ssa[];	/* roctets  source ATM subaddress */
	u_char	ar_spa[];	/* soctets  source protocol address */
	u_char	ar_tha[];	/* xoctets  target ATM number */
	u_char	ar_tsa[];	/* yoctets  target ATM subaddress */
	u_char	ar_tpa[];	/* zoctets  target protocol address */
#endif
};

static u_char ezero[6];

#define	ATMARP_TL_E164	0x40	/* Is an E164 address */
#define	ATMARP_TL_LEN	0x3f	/* Length mask */

static int octet_clusters[] = { 1, 2, 1, 3, 2, 2, 2, 6 , 1 };

static char *
arp_atm_fmt(u_int tl, u_char *ap, u_int stl, u_char *sap)
{
	u_int i, len;
	static char buf[100];
	char *bp;
	
	bp = buf;

	len = tl & ATMARP_TL_LEN;
	if (len != 0) {
		if (tl & ATMARP_TL_E164)
			for (i = 0; i < len; i++)
				bp += snprintf(bp, sizeof(buf) - (bp - buf),
				    "%02x:", ap[i]);
		else
			bp += snprintf(bp, sizeof(buf) - (bp - buf),
			    "0x%02x.%02x%02x.%02x.%02x%02x%02x.%02x%02x."
			    "%02x%02x.%02x%02x.%02x%02x%02x%02x%02x%02x.%02x",
			    ap[0], ap[1], ap[2], ap[3], ap[4],
			    ap[5], ap[6], ap[7], ap[8], ap[9],
			    ap[10], ap[11], ap[12], ap[13], ap[14],
			    ap[15], ap[16], ap[17], ap[18], ap[19]);
	}
	len = stl & ATMARP_TL_LEN;
	if (len != 0) {
		*bp++ = '/';
		if (stl & ATMARP_TL_E164)
			for (i = 0; i < len; i++)
				bp += snprintf(bp, sizeof(buf) - (bp - buf),
				    "%02x:", sap[i]);
		else
			bp += snprintf(bp, sizeof(buf) - (bp - buf),
			    "0x%02x.%02x%02x.%02x.%02x%02x%02x.%02x%02x."
			    "%02x%02x.%02x%02x.%02x%02x%02x%02x%02x%02x.%02x",
			    sap[0], sap[1], sap[2], sap[3], sap[4],
			    sap[5], sap[6], sap[7], sap[8], sap[9],
			    sap[10], sap[11], sap[12], sap[13], sap[14],
			    sap[15], sap[16], sap[17], sap[18], sap[19]);
	}

	return (buf);
}

static void
arp_atm_print(register const u_char *bp, u_int length, u_int caplen)
{
	register const struct atmarphdr *ap;
	register u_short pro, hrd, op;
	register u_char shl, ssl, thl, tsl;
	register u_char *sha, *ssa, *spa, *tha, *tsa, *tpa;

	ap = (struct atmarphdr *)bp;
	if ((u_char *)(ap + 1) > snapend) {
	Trunc:
		printf("[|atmarp]");
		return;
	}
	if (length < sizeof(struct atmarphdr)) {
		(void)printf("truncated-atmarp");
		default_print((u_char *)ap, length);
		return;
	}

	pro = EXTRACT_16BITS(&ap->ar_pro);
	hrd = EXTRACT_16BITS(&ap->ar_hrd);
	op = EXTRACT_16BITS(&ap->ar_op);

	shl = ap->ar_shtl & ATMARP_TL_LEN;
	ssl = ap->ar_sstl & ATMARP_TL_LEN;
	thl = ap->ar_thtl & ATMARP_TL_LEN;
	tsl = ap->ar_tstl & ATMARP_TL_LEN;
	sha = (u_char *)(ap + 1);
	ssa = sha + shl;
	spa = ssa + ssl;
	tha = spa + ap->ar_spln;
	tsa = tha + thl;
	tpa = tsa + tsl;

	if (tpa + ap->ar_tpln > snapend)
		goto Trunc;

	if (pro != ETHERTYPE_IP) {
		(void)printf("atmarp-#%d for proto #%d hardware #%d",
				op, pro, hrd);
		return;
	}

	switch (op) {
	case ARPOP_REQUEST:
		(void)printf("atmarp who-has %s", ipaddr_string(tpa));
		if (thl || tsl)
			(void)printf(" (%s)", arp_atm_fmt(thl, tha, tsl, tsa));
		(void)printf(" tell %s", ipaddr_string(spa));
		if (shl || ssl)
			(void)printf(" (%s)", arp_atm_fmt(shl, sha, ssl, ssa));
		break;

	case ARPOP_REPLY:
		(void)printf("atmarp reply %s is-at %s", 
		    ipaddr_string(spa), arp_atm_fmt(shl, sha, ssl, ssa));
		(void)printf(" to %s", ipaddr_string(tpa));
		if (thl || tsl)
			(void)printf(" (%s)", arp_atm_fmt(thl, tha, tsl, tsa));
		break;

	case REVARP_REQUEST:
		(void)printf("ratmarp who-is %s", 
		    arp_atm_fmt(thl, tha, tsl, tsa));
		(void)printf(" tell %s (%s)", 
		    arp_atm_fmt(shl, sha, ssl, ssa),
		    ipaddr_string(spa));
		break;

	case REVARP_REPLY:
		(void)printf("ratmarp reply %s at %s",
			arp_atm_fmt(thl, tha, tsl, tsa),
			ipaddr_string(tpa));
		(void)printf(" to %s", ipaddr_string(spa));
		if (shl || ssl)
			(void)printf(" (%s)", arp_atm_fmt(shl, sha, ssl, ssa));
		break;

	case INARP_REQUEST:
		(void)printf("inatmarp who-is %s", 
		    arp_atm_fmt(thl, tha, tsl, tsa));
		if (ap->ar_tpln)
			(void)printf(" (%s)", ipaddr_string(tpa));
		(void)printf(" tell %s", ipaddr_string(spa));
		if (shl || ssl)
			(void)printf(" (%s)", arp_atm_fmt(shl, sha, ssl, ssa));
		break;

	case INARP_REPLY:
		(void)printf("inatmarp reply %s", arp_atm_fmt(thl, tha, tsl, tsa));
		(void)printf(" is %s", ipaddr_string(tpa));
		(void)printf(" to %s", ipaddr_string(spa));
		if (thl || tsl)
			(void)printf(" (%s)", arp_atm_fmt(shl, sha, ssl, ssa));
		break;

	case ARP_NAK:
		(void)printf("atmarp nak don't-know %s", ipaddr_string(tpa));
		if (thl || tsl)
			(void)printf(" (%s)", arp_atm_fmt(thl, tha, tsl, tsa));
		(void)printf(" to %s", ipaddr_string(spa));
		if (shl || ssl)
			(void)printf(" (%s)", arp_atm_fmt(shl, sha, ssl, ssa));
		break;

	default:
		(void)printf("atmarp-#%d", op);
		default_print((u_char *)ap, caplen);
		return;
	}
}

void
arp_print(register const u_char *bp, u_int length, u_int caplen)
{
	register const struct ether_arp *ap;
	register const struct ether_header *eh;
	register u_short pro, hrd, op;

	ap = (struct ether_arp *)bp;
	if ((u_char *)(ap + 1) > snapend) {
		printf("[|arp]");
		return;
	}
	if (length < sizeof(struct ether_arp)) {
		(void)printf("truncated-arp");
		default_print((u_char *)ap, length);
		return;
	}

	pro = EXTRACT_16BITS(&ap->arp_pro);
	hrd = EXTRACT_16BITS(&ap->arp_hrd);
	op = EXTRACT_16BITS(&ap->arp_op);

	if (hrd == ARPHDR_ATM) {
		arp_atm_print(bp, length, caplen);
		return;
	}

	if ((pro != ETHERTYPE_IP && pro != ETHERTYPE_TRAIL)
	    || ap->arp_hln != sizeof(SHA(ap))
	    || ap->arp_pln != sizeof(SPA(ap))) {
		(void)printf("arp-#%d for proto #%d (%d) hardware #%d (%d)",
				op, pro, ap->arp_pln,
				hrd, ap->arp_hln);
		return;
	}
	if (pro == ETHERTYPE_TRAIL)
		(void)printf("trailer-");
	eh = (struct ether_header *)packetp;
	switch (op) {

	case ARPOP_REQUEST:
		(void)printf("arp who-has %s", ipaddr_string(TPA(ap)));
		if (memcmp((char *)ezero, (char *)THA(ap), 6) != 0)
			(void)printf(" (%s)", etheraddr_string(THA(ap)));
		(void)printf(" tell %s", ipaddr_string(SPA(ap)));
		if (memcmp((char *)ESRC(eh), (char *)SHA(ap), 6) != 0)
			(void)printf(" (%s)", etheraddr_string(SHA(ap)));
		break;

	case ARPOP_REPLY:
		(void)printf("arp reply %s", ipaddr_string(SPA(ap)));
		if (memcmp((char *)ESRC(eh), (char *)SHA(ap), 6) != 0)
			(void)printf(" (%s)", etheraddr_string(SHA(ap)));
		(void)printf(" is-at %s", etheraddr_string(SHA(ap)));
		if (memcmp((char *)EDST(eh), (char *)THA(ap), 6) != 0)
			(void)printf(" (%s)", etheraddr_string(THA(ap)));
		break;

	case REVARP_REQUEST:
		(void)printf("rarp who-is %s tell %s",
			etheraddr_string(THA(ap)),
			etheraddr_string(SHA(ap)));
		break;

	case REVARP_REPLY:
		(void)printf("rarp reply %s at %s",
			etheraddr_string(THA(ap)),
			ipaddr_string(TPA(ap)));
		break;

	default:
		(void)printf("arp-#%d", op);
		default_print((u_char *)ap, caplen);
		return;
	}
	if (hrd != ARPHRD_ETHER)
		printf(" hardware #%d", hrd);
}

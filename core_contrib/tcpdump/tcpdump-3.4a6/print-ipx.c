/*
 * Copyright (c) 1994, 1995, 1996
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
 *
 * Format and print Novell IPX packets.
 * Contributed by Brad Parker (brad@fcr.com).
 */

#ifndef lint
static const char rcsid[] =
    "@(#) /master/core_contrib/tcpdump/tcpdump-3.4a6/print-ipx.c,v 1.3 2000/11/22 11:05:04 billh Exp";
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>

#ifdef __STDC__
#include <stdlib.h>
#endif
#include <stdio.h>
#include <string.h>

#include "interface.h"
#include "addrtoname.h"
#include "ipx.h"
#include "extract.h"


static const char *ipxaddr_string(u_int32_t, const u_char *);
void ipx_decode(const struct ipxHdr *, const u_char *, u_int);
void ipx_sap_print(const u_short *, u_int);
void ipx_sap_dprint(const sap_hdr_t *sap, int length);
void ipx_rip_print(const u_short *, u_int);
void ipx_rip_dprint(const rip_hdr_t *hdr, int len);

/*
 * Print IPX datagram packets.
 */
void
ipx_print(const u_char *p, u_int length)
{
	const struct ipxHdr *ipx = (const struct ipxHdr *)p;

	TCHECK(ipx->srcSkt);
	(void)printf("%s.%x > ",
		     ipxaddr_string(EXTRACT_32BITS(ipx->srcNet), ipx->srcNode),
		     EXTRACT_16BITS(&ipx->srcSkt));

	(void)printf("%s.%x:",
		     ipxaddr_string(EXTRACT_32BITS(ipx->dstNet), ipx->dstNode),
		     EXTRACT_16BITS(&ipx->dstSkt));

	/* take length from ipx header */
	TCHECK(ipx->length);
	length = EXTRACT_16BITS(&ipx->length);

	ipx_decode(ipx, (u_char *)ipx + ipxSize, length - ipxSize);
	return;
trunc:
	printf("[|ipx %d]", length);
}

static const char *
ipxaddr_string(u_int32_t net, const u_char *node)
{
    static char line[256];

    snprintf(line, sizeof(line), "%x.%02x:%02x:%02x:%02x:%02x:%02x",
	    net, node[0], node[1], node[2], node[3], node[4], node[5]);

    return line;
}

void
ipx_decode(const struct ipxHdr *ipx, const u_char *datap, u_int length)
{
    register u_short dstSkt;
    register u_short srcSkt;

    dstSkt = EXTRACT_16BITS(&ipx->dstSkt);
    switch (dstSkt) {
      case IPX_SKT_NCP:
	(void)printf(" ipx-ncp %d", length);
	break;
      case IPX_SKT_SAP:
	ipx_sap_print((u_short *)datap, length);
	break;
      case IPX_SKT_RIP:
	ipx_rip_print((u_short *)datap, length);
	break;
      case IPX_SKT_NETBIOS:
	(void)printf(" ipx-netbios %d", length);
	break;
      case IPX_SKT_DIAGNOSTICS:
	(void)printf(" ipx-diags %d", length);
	break;
      default:
	if (EXTRACT_16BITS(&ipx->srcSkt) == IPX_SKT_SAP)
          ipx_sap_print((u_short *)datap, length);
	else
	  (void)printf(" ipx-#%x %d", dstSkt, length);
	break;
    }
}

void
ipx_sap_print(const u_short *ipx, u_int length)
{
    int command, i;

    TCHECK(ipx[0]);

    if (vflag > 1) {
      ipx_sap_dprint((sap_hdr_t *)ipx, length);
      return;
    }

    command = EXTRACT_16BITS(ipx);
    ipx++;
    length -= 2;

    switch (command) {
      case 1:
      case 3:
	if (command == 1)
	    (void)printf("ipx-sap-general-req");
	else
	    (void)printf("ipx-sap-nearest-req");

	if (length > 0) {
	    TCHECK(ipx[1]);
	    (void)printf(" %x '", EXTRACT_16BITS(&ipx[0]));
	    fn_print((u_char *)&ipx[1], (u_char *)&ipx[1] + 48);
	    putchar('\'');
	}
	break;

      case 2:
      case 4:
	if (command == 2)
	    (void)printf("ipx-sap-general-resp");
	else
	    (void)printf("ipx-sap-nearest-resp");

	for (i = 0; i < 8 && length > 0; i++) {
	    TCHECK2(ipx[27], 1);
	    (void)printf(" %x '", EXTRACT_16BITS(&ipx[0]));
	    fn_print((u_char *)&ipx[1], (u_char *)&ipx[1] + 48);
	    printf("' addr %s",
		ipxaddr_string(EXTRACT_32BITS(&ipx[25]), (u_char *)&ipx[27]));
	    ipx += 32;
	    length -= 64;
	}
	break;
      default:
	    (void)printf("ipx-sap-?%x", command);
	break;
    }
	return;
trunc:
	printf("[|ipx %d]", length);
}

char *sap_cmd_name[] = {
	"General request",
	"General response",
	"Nearest request",
	"Nearest response"
};

struct sap_srvtypes {
	int type;
	char *name;
} sap_srvtypes[] = {
	{ 0x0004, "File server" },
	{ 0x0005, "Job server" },
	{ 0x0007, "Print server" },
	{ 0x0009, "Archive server" },
	{ 0x000a, "Job queue" },
	{ 0x0021, "NAS SNA gateway" },
	{ 0x002e, "Dynamic SAP" },
	{ 0x0047, "Advertising print server" },
	{ 0x004b, "Btrieve VAP 5.0" },
	{ 0x004c, "SQL VAP" },
	{ 0x007a, "TES - NetWare VMS" },
	{ 0x0098, "NetWare access server" },
	{ 0x009a, "Named pipes server" },
	{ 0x009e, "Portable NetWare - Unix" },
	{ 0x0107, "NetWare 386" },
	{ 0x0111, "Test server" },
	{ 0x0166, "NetWare management" },
	{ 0x026a, "NetWare management" },
	{ 0x026b, "Time synchronization" },
	{ 0x0278, "NetWare directory server" },
	{ 0x0840, "Internet server" },
	{ 0, NULL }
};

/* Print lots of detail */
void
ipx_sap_dprint(const sap_hdr_t *sap, int len)
{
	u_short t;
	struct sap_srvtypes *tp;
	sap_rec_t *sr;

	t = EXTRACT_16BITS(sap->cmd);
	printf(" ipx-sap:\n\tCommand : %4.4x", t);
	if (t >= 1 && t <= 4)
		printf(" (%s)", sap_cmd_name[t - 1]);
	else
		printf(" (???)");
	t = EXTRACT_16BITS(sap->srvtype);
	printf(   "\n\tSrvType : %4.4x", t);
	for (tp = sap_srvtypes; tp->type != 0; tp++)
		if (tp->type == t) break;
	if (tp->type != 0)
		printf(" (%s)", tp->name);
	len -= sizeof(sap_hdr_t);
	sr = (sap_rec_t *)(sap + 1);
	while (len >= sizeof(sap_rec_t)) {
		printf("\n\t  %30s.%4.4x hops=%4.4x %s",
		     ipxaddr_string(EXTRACT_32BITS(sr->net), sr->node),
		     EXTRACT_16BITS(sr->socket),
		     EXTRACT_16BITS(sr->hops),
		     sr->name);
		len -= sizeof(sap_rec_t);
		sr++;
	}
	if (len != 0) {
		printf("\n\tTrailing data:");
		hexdump(sr, len);
	}
}

void
ipx_rip_print(const u_short *ipx, u_int length)
{
    int command, i;

    TCHECK(ipx[0]);

    if (vflag > 1) {
      ipx_rip_dprint((rip_hdr_t *)ipx, length);
      return;
    }

    command = EXTRACT_16BITS(ipx);
    ipx++;
    length -= 2;

    switch (command) {
      case 1:
	(void)printf("ipx-rip-req");
	if (length > 0) {
	    TCHECK(ipx[3]);
	    (void)printf(" %u/%d.%d", EXTRACT_32BITS(&ipx[0]),
			 EXTRACT_16BITS(&ipx[2]), EXTRACT_16BITS(&ipx[3]));
	}
	break;
      case 2:
	(void)printf("ipx-rip-resp");
	for (i = 0; i < 50 && length > 0; i++) {
	    TCHECK(ipx[3]);
	    (void)printf(" %u/%d.%d", EXTRACT_32BITS(&ipx[0]),
			 EXTRACT_16BITS(&ipx[2]), EXTRACT_16BITS(&ipx[3]));

	    ipx += 4;
	    length -= 8;
	}
	break;
      default:
	    (void)printf("ipx-rip-?%x", command);
    }
	return;
trunc:
	printf("[|ipx %d]", length);
}

void
ipx_rip_dprint(const rip_hdr_t *hdr, int len)
{
	u_short t;
	rip_rec_t *sr;

	t = EXTRACT_16BITS(hdr->type);
	switch (t) {
	case RIP_REQUEST:
		printf(" ipx-rip-request:");
		break;
	case RIP_RESPONSE:
		printf(" ipx-rip-response:");
		break;
	default:
		printf(" ipx-rip-??? (%x)", t);
	}
	len -= sizeof(rip_hdr_t);
	sr = (rip_rec_t *)(hdr + 1);
	if (len >= sizeof(rip_rec_t))
		printf("\n\t  Net      Hops Ticks");
	while (len >= sizeof(rip_rec_t)) {
		printf("\n\t  %8.8x %4.4x %4.4x",
			EXTRACT_32BITS(sr->net),
			EXTRACT_16BITS(sr->hops),
			EXTRACT_16BITS(sr->ticks));
		len -= sizeof(rip_rec_t);
		sr++;
	}
	if (len != 0) {
		printf("\n\tTrailing data:");
		hexdump(sr, len);
	}
}

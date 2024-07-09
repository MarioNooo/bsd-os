/*
 * This file has been slightly modified by NRL for use with IPv6+IPsec.
 * Search for INET6 and/or IPSEC to see the blocks where this happened.
 * See the NRL Copyright notice for conditions on the modifications.
 */
/*
 * Copyright (c) 1990, 1991, 1993, 1994, 1995, 1996, 1997
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
    "@(#) /master/core_contrib/tcpdump/tcpdump-3.4a6/print-ppp.c,v 1.3 1998/02/17 14:47:38 jch Exp (LBL)";
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

#include <ctype.h>
#include <netdb.h>
#include <pcap.h>
#include <stdio.h>

#include <net/slcompress.h>
#include <net/slip.h>

#include "interface.h"
#include "addrtoname.h"
#include "extract.h"
#include "ppp.h"

/* XXX This goes somewhere else. */
#define PPP_HDRLEN 4

/* Standard PPP printer */
void
ppp_if_print(u_char *user, const struct pcap_pkthdr *h,
	     register const u_char *p)
{
	register u_int length = h->len;
	register u_int caplen = h->caplen;
	const struct ip *ip;

	ts_print(&h->ts);

	if (caplen < PPP_HDRLEN) {
		printf("[|ppp]");
		goto out;
	}

	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	packetp = p;
	snapend = p + caplen;

	if (eflag)
		printf("%c %4d %02x %04x: ", p[0] ? 'O' : 'I', length,
		       p[1], ntohs(*(u_short *)&p[2]));

	length -= PPP_HDRLEN;
	ip = (struct ip *)(p + PPP_HDRLEN);
	if (ip->ip_v == 4) 
		ip_print((const u_char *)ip, length);
#ifdef	INET6
        else  
		ipv6_print((const u_char *)ip, length);
#endif

	if (xflag)
		default_print((const u_char *)ip, caplen - PPP_HDRLEN);
out:
	putchar('\n');
}

/* proto type to string mapping */

static struct tok ptype2str[] = {
	{ PPP_IP,	"IP" },
	{ PPP_OSI,	"OSI" },
	{ PPP_NS,	"NS" },
	{ PPP_DECNET,	"DECNET" },
	{ PPP_APPLE,	"APPLE" },
	{ PPP_IPX,	"IPX" },
	{ PPP_VJC,	"VJC" },
	{ PPP_VJNC,	"VJNC" },
	{ PPP_BRPDU,	"BRPDU" },
	{ PPP_STII,	"STII" },
	{ PPP_VINES,	"VINES" },
	{ PPP_IPV6,	"IPV6" },

	{ PPP_HELLO,	"HELLO" },
	{ PPP_LUXCOM,	"LUXCOM" },
	{ PPP_SNS,	"SNS" },

	{ PPP_IPCP,	"IPCP" },
	{ PPP_OSICP,	"OSICP" },
	{ PPP_NSCP,	"NSCP" },
	{ PPP_DECNETCP,	"DECNETCP" },
	{ PPP_APPLECP,	"APPLECP" },
	{ PPP_IPXCP,	"IPXCP" },
	{ PPP_STIICP,	"STIICP" },
	{ PPP_VINESCP,	"VINESCP" },

	{ PPP_LCP,	"LCP" },
	{ PPP_PAP,	"PAP" },
	{ PPP_LQR,	"LQR" },
	{ PPP_CHAP,	"CHAP" },
	{ 0,		NULL }
};

static void ppp_lcp_print(const u_char *, int);
static void ppp_ipcp_print(const u_char *, int);
static void ppp_pap_print(const u_char *, int);
static void ppp_chap_print(const u_char *, int);

/* BSD/OS specific PPP printer */
void
ppp_bsdos_if_print(u_char *user, const struct pcap_pkthdr *h,
	     register const u_char *p)
{
	register u_int length = h->len;
	register u_int caplen = h->caplen;
	register int hdrlength;
	u_short ptype;
	u_char address, control, dir;
	const u_char *ch;
	char *direction;

	ts_print(&h->ts);

	if (caplen < SLC_BPFHDR) {
		printf("[|ppp]");
		goto out;
	}

	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	packetp = p;
	snapend = p + caplen;

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

	/* Decode the BPF header */
	direction = (dir = *p++ == SLIPDIR_IN) ? "I " : "O ";
	hdrlength = *p++;
	p++;	/* Skip compressed header length */
	/* Decode the compressed PPP header */
	if (hdrlength > 2) {
		address = *p++;
		control = *p++;
	} else {
		address = PPP_ADDRESS;
		control = PPP_CONTROL;
	}
	if (hdrlength & 0x01)
		ptype = *p++;
	else {
		ptype = EXTRACT_16BITS(p);
		p += 2;
	}
	/* Save pointer to compressed header */
	ch = p;
	if (eflag) {
		printf("%s", direction);
		direction = "";
		printf("%02x %02x %d ", address, control, length);
		if (hdrlength & 0x01)
			printf("%02x ", ptype);
		else
			printf("%04x ", ptype);
	}

	length -= SLC_BPFHDR;
	p = packetp + SLC_BPFHDR;

	switch (ptype) {
	case PPP_VJC:
		if (eflag)
			compressed_sl_print(ch, (struct ip *)p, length, dir);
		ip_print(p, length);
		break;
	case PPP_VJNC:
		if (eflag)
			uncompressed_sl_print(ch[1], (struct ip *)p, length,
			    dir);
		ip_print(p, length);
		break;
	case PPP_IP:
		ip_print(p, length);
		break;
#ifdef	INET6
	case PPP_IPV6:
		ipv6_print(p, length);
		break;
#endif
	case PPP_DECNET:
		decnet_print(p, length, caplen);
		break;
	case PPP_APPLE:
		atalk_print(p, length);
		break;

	case PPP_NS:	/* XXX?? */
	case PPP_IPX:
		ipx_print(p, length);
		break;

	case PPP_IPCP:
		printf("%sipcp ", direction);
		ppp_ipcp_print(p, length);
		break;
	case PPP_LCP:
		printf("%slcp ", direction);
		ppp_lcp_print(p, length);
		break;
        case PPP_PAP:
		printf("%spap ", direction);
		ppp_pap_print(p, length);
		break;
	case PPP_LQR:
		printf("%slqr ", direction);
		break;
	case PPP_CHAP:
		printf("%schap ", direction);
		ppp_chap_print(p, length);
		break;

	default:
		if (!eflag)
			printf("%s ", tok2str(ptype2str, "proto-#%d", ptype));
		break;
	}

	if (xflag)
		default_print((const u_char *)p, caplen - SLC_BPFHDR);
out:
	if (xxflag)
		hexdump(p, caplen - SLC_BPFHDR);
	putchar('\n');
}

static void
ppp_lcp_print(const u_char *cp, int length)
{
	register const u_char *lp;
	int len;

	if (cp + PPP_CPHDR_LEN > snapend) {
		printf("[|lcp]");
		return;
	}
	if (length < PPP_CPHDR_LEN) {
		printf("truncated-lcp %u", length);
		return;
	}

	printf("id %u ", cp[PPP_CP_ID]);
	len = EXTRACT_16BITS(cp + PPP_CP_LEN);
	lp = min(cp + len, snapend);

	switch (cp[PPP_CP_CODE]) {
		const u_char *op;
		u_short s;
		
	case PPP_CP_CONF_REQ: 
		printf("conf-req"); 
		goto Options;
	case PPP_CP_CONF_ACK: 
		printf("conf-ack"); 
		goto Options;
	case PPP_CP_CONF_NAK: 
		printf("conf-nak"); 
		goto Options;
	case PPP_CP_CONF_REJ: 
		printf("conf-rej");
	    /* FALL THROUGH */
	Options:
		for (op = cp + PPP_CPHDR_LEN; op < lp;
		     op = op + op[PPP_OP_LEN]) {
			if (op + op[PPP_OP_LEN] > lp) {
				printf("[|lcp]");
				return;
			}
		 	switch (op[PPP_OP_TYPE]) {
			case PPP_LCP_MRU:
				printf(" {mru %u}",
				    EXTRACT_16BITS(op + PPP_OP_DATA));
				break;
			case PPP_LCP_ACCM:
				printf(" {accm %08x}",
				    EXTRACT_32BITS(op + PPP_OP_DATA));
				break;
			case PPP_LCP_AP:
				s = EXTRACT_16BITS(op + PPP_OP_DATA);
				printf(" {ap ");
				switch (s) {
				case PPP_PAP:
					printf("pap}");
					break;

				case PPP_CHAP:
					printf("chap w/");
					switch (op[PPP_OP_DATA + 2]) {
					case PPP_CHAPA_MD5:
						printf("md5}");
						break;

					default:
						printf("%02x}",
						    op[PPP_OP_DATA + 2]);
						break;
					}
					break;

				default:
					printf("%04x}", s);
					break;
				}
				break;
			case PPP_LCP_QP:
				printf(" {qp %ums",
				    EXTRACT_32BITS(op + PPP_OP_DATA) * 10);
				s = EXTRACT_16BITS(op + PPP_OP_DATA);
				switch (s) {
				case PPP_LQR:
					printf("lqr}");
					break;

				default:
					printf("%04x}", s);
				}
				break;
			case PPP_LCP_MAGIC:
				printf(" {magic %x}",
				    EXTRACT_32BITS(op + PPP_OP_DATA));
				break;
			case PPP_LCP_PFC:
				printf(" {pfc}");
				break;
			case PPP_LCP_ACFC:
				printf(" {acfc}");
				break;
			default:
				printf(" {option-%d %d}",
				    op[PPP_OP_TYPE], op[PPP_OP_LEN]);
				break;
			}
		}
		break;
		

	case PPP_CP_TERM_REQ: 
		printf("term-req"); 
		goto Terminate;
	case PPP_CP_TERM_ACK: 
		printf("term-ack");
	Terminate:
		op = cp + PPP_CPHDR_LEN;
		goto Data;

	case PPP_CP_CODE_REJ:
		printf("code-rej ");
		ppp_lcp_print(cp + PPP_CPHDR_LEN, len);
		break;

	case PPP_LCP_PROTO_REJ:
		printf("proto-rej proto %s", tok2str(ptype2str, "#%d",
		    EXTRACT_16BITS(cp + PPP_CPHDR_LEN)));
		break;

	case PPP_LCP_ECHO_REQ: 
		printf("echo-req"); 
		goto Magic;
	case PPP_LCP_ECHO_REPL: 
		printf("echo-rpl"); 
		goto Magic;
	case PPP_LCP_DISC_REQ: 
		printf("discard-req"); 
		goto Magic;
	case PPP_LCP_IDENT: 
		printf("ident");
	Magic:
		printf(" magic %x", EXTRACT_32BITS(cp + PPP_CPHDR_LEN));
		op = cp + PPP_CPHDR_LEN + sizeof (u_long);
	Data:
		if (vflag && op < lp) {
			printf(" data");
			for (; op < lp - 1; op += 2)
				printf(" %02x%02x", op[0], op[1]);
			if (op < lp)
				printf(" %02x", *op);
		}
	    	break;

	case PPP_LCP_TIME_REMAIN:
		op = cp + PPP_CPHDR_LEN;
		printf("time remaining %u",
		    EXTRACT_32BITS(op + 4));
		printf(" magic %x message %.*s", EXTRACT_32BITS(op + 4),
		    lp - (op + 8), op + 8);
		break;

	default:
		printf("lcp-code-%d %d", cp[PPP_CP_CODE], length);
		break;
	}
}

static void
ppp_ipcp_print(const u_char *cp, int length)
{
	register const u_char *lp;
	int len;

	if (cp + PPP_CPHDR_LEN > snapend) {
		printf("[|ipcp]");
		return;
	}
	if (length < PPP_CPHDR_LEN) {
		printf("truncated-lcp %u", length);
		return;
	}

	printf("id %u ", cp[PPP_CP_ID]);
	len = EXTRACT_16BITS(cp + PPP_CP_LEN);
	lp = min((u_char *) cp + len, snapend);

	switch (cp[PPP_CP_CODE]) {
		const u_char *op;
		
	case PPP_CP_CONF_REQ:
		printf("conf-req"); 
		goto Options;
	case PPP_CP_CONF_ACK: 
		printf("conf-ack"); 
		goto Options;
	case PPP_CP_CONF_NAK: 
		printf("conf-nak"); 
		goto Options;
	case PPP_CP_CONF_REJ: 
		printf("conf-rej");
	    /* FALL THROUGH */
	Options:
		for (op = cp + PPP_CPHDR_LEN; op < lp;
		     op = op + op[PPP_OP_LEN]) {
		 	switch (op[PPP_OP_TYPE]) {
				u_short s;
			    
			case PPP_IPCP_ADDRS:
				printf(" {addrs lcl %s rmt %s}",
				    ipaddr_string(op + PPP_OP_DATA),
				    ipaddr_string(op + PPP_OP_DATA + 4));
				break;

			case PPP_IPCP_CPROT:
				s = EXTRACT_16BITS(op + PPP_OP_DATA);
				printf(" {cprot ");
				switch (s) {
				case PPP_VJC:
					printf("vj");
					break;

				default:
					printf("%04x", s);
					break;
				}
				if (vflag) {
					const u_char *dp, *dlp;

					dlp = min(op + op[PPP_OP_LEN], lp);
					for (dp = op + PPP_OP_LEN + 2; dp < dlp -1; dp += 2)
				    		printf(" %02x%02x", dp[0], dp[1]);
					if (dp < dlp)
						printf(" %02x", *dp);
				}
				putchar('}');
				break;

			case PPP_IPCP_ADDR:
				printf(" {addr %s}",
				    ipaddr_string(op + PPP_OP_DATA));
				break;

			case PPP_IPCP_PDNS:
				printf(" {dns-primary %s}",
				    ipaddr_string(op + PPP_OP_DATA));
				break;

			case PPP_IPCP_PNBNS:
				printf(" {nbns-primary %s}",
				    ipaddr_string(op + PPP_OP_DATA));
				break;

			case PPP_IPCP_SDNS:
				printf(" {dns-secondary %s}",
				    ipaddr_string(op + PPP_OP_DATA));
				break;

			case PPP_IPCP_SNBNS:
				printf(" {nbns-secondary %s}",
				    ipaddr_string(op + PPP_OP_DATA));
				break;

			default:
				printf(" {option-%d %d}",
				    op[PPP_OP_TYPE], op[PPP_OP_LEN]);
				break;
			}
		}
		break;
		

	case PPP_CP_TERM_REQ: 
		printf("term-req"); 
		goto Terminate;
	case PPP_CP_TERM_ACK: 
		printf("term-ack");
	Terminate:
		op = cp + PPP_CPHDR_LEN;
		if (vflag && op < lp) {
			printf(" data");
			for (; op < lp - 1; op += 2)
				printf(" %02x%02x", op[0], op[1]);
			if (op < lp)
				printf(" %02x", *op);
		}
	    	break;

	case PPP_CP_CODE_REJ: 
		printf("code-rej"); 
		break;

	default:
		printf("ipcp-code-%d %d", cp[PPP_CP_CODE], length);
		break;
	}
}


static void
ppp_pap_print(const u_char *cp, int length)
{
	register const u_char *lp, *op;
	int len;

	if (cp + PPP_CPHDR_LEN > snapend) {
		printf("[|ipcp]");
		return;
	}
	if (length < PPP_CPHDR_LEN) {
		printf("truncated-lcp %u", length);
		return;
	}

	printf("id %u ", cp[PPP_CP_ID]);
	len = EXTRACT_16BITS(cp + PPP_CP_LEN);
	lp = min((u_char *) cp + len, snapend);
	op = cp + PPP_CPHDR_LEN;

	switch (cp[PPP_CP_CODE]) {
	case PPP_PAP_AUTH_REQ:
		printf("auth-req");
		if (vflag) {
		    	const u_char *dp, *dlp;

			/* Peer-id */
			printf(" peer-id");
			for (dp = op + 1, dlp = min(dp + *op, lp);
			     dp < dlp - 1;
			     dp += 2)
				printf(" %02x%02x", dp[0], dp[1]);
			if (dp < dlp)
				printf(" %02x", *dp);
			op += *op + 1;
			if (op >= lp)
				break;

			/* Password */
			printf(" passwd");
			for (dp = op + 1, dlp = min(dp + *op, lp);
			     dp < dlp - 1;
			     dp += 2)
				printf(" %02x%02x", dp[0], dp[1]);
			if (dp < dlp)
				printf(" %02x", *dp);
		}
		break;

	case PPP_PAP_AUTH_ACK: 
		printf("auth-ack"); 
		goto Message;
	case PPP_PAP_AUTH_NAK: 
		printf("auth-nak");
	Message:
		if (vflag && op[0]) {
			op++;
			printf(" msg %.*s", lp - op, op);
		}
		break;

	default:
		printf("pap-code-%d %d", cp[PPP_CP_CODE], length);
		break;
	}
}

static void
ppp_chap_print(const u_char *cp, int length)
{
	register const u_char *lp, *op;
	int len;

	if (cp + PPP_CPHDR_LEN > snapend) {
		printf("[|ipcp]");
		return;
	}
	if (length < PPP_CPHDR_LEN) {
		printf("truncated-lcp %u", length);
		return;
	}

	printf("id %u ", cp[PPP_CP_ID]);
	len = EXTRACT_16BITS(cp + PPP_CP_LEN);
	lp = min((u_char *) cp + len, snapend);
	op = cp + PPP_CPHDR_LEN;

	switch (cp[PPP_CP_CODE]) {
	case PPP_CHAP_CHALLANGE: 
		printf("challange"); 
		goto Value;
	case PPP_CHAP_RESPONSE: 
		printf("response");
	Value:
		if (vflag) {
		    	const u_char *dp, *dlp;

			/* Value */
			printf(" value");
			for (dp = op + 1, dlp = dp + *op;
			     dp < dlp - 1;
			     dp += 2)
				printf(" %02x%02x", dp[0], dp[1]);
			if (dp < dlp)
				printf(" %02x", *dp);
			op += *op + 1;

			/* Name */
			printf(" name");
			for (dp = op, dlp = lp;
			     dp < dlp - 1;
			     dp += 2)
				printf(" %02x%02x", dp[0], dp[1]);
			if (dp < dlp)
				printf(" %02x", *dp);
		}
		break;
	    
	case PPP_CHAP_SUCCESS: 
		printf("success"); 
		goto Message;
	case PPP_CHAP_FAILURE: 
		printf("failure");
 Message:
		if (vflag) {
		    	const u_char *dp, *dlp;

			/* Message */
			if (vflag)
				printf(" msg %.*s", lp - op, op);
		}
		break;

	default:
		printf("chap-code-%d %d", cp[PPP_CP_CODE], length);
		break;
	}
}

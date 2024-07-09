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
    "@(#)/master/core_contrib/tcpdump/tcpdump-3.4a6/print-chdlc.c,v 1.3 2000/02/18 22:22:50 jch Exp (LBL)";
#endif

#ifdef	HAVE_C_HDLC

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
#include <net/if_c_hdlc.h>

#include <ctype.h>
#include <stdio.h>

#include "interface.h"
#include "ethertype.h"
#include "addrtoname.h"
#include "extract.h"
#include "pcap.h"

#define	CHDLC_HDRLEN	sizeof (struct cisco_hdr)
#define	CHDLC_SLARP_HDRLEN	SLARP_SIZE

static void
chdlc_slarp_print(const u_char *pp, int length)
{
	const struct cisco_slarp *slp = (struct cisco_slarp *) pp;
	const char *slarp = eflag ? "" : "slarp: ";

	if (length < SLARP_SIZE) {
		printf("[|slarp]");
		return;
	}

	switch (ntohl(slp->csl_code)) {
	case SLARP_REQUEST:
		printf("%srequest",
		       slarp);
		break;

	case SLARP_REPLY:
		printf("%sreply %s/%s",
		       slarp,
		       ipaddr_string(&slp->csl_addr),
		       ipaddr_string(&slp->csl_mask));
		break;

	case SLARP_KEEPALIVE:
		if (qflag) {
			printf("%skeepalive",
			       slarp);
		} else {
			printf("%skeepalive myseq %#08lx yourseq %#08lx",
			       slarp,
			       ntohl(slp->csl_myseq),
			       ntohl(slp->csl_yourseq));
			if (vflag) {
			    u_long t = ((ntohs(slp->csl_t1) << 16) + ntohs(slp->csl_t0));

			    (void) printf(" %u:%02u:%02u.%03u ",
					  t / (60*60*1000),
					  (t / (60*1000)) % 60,
					  (t / 1000) % 60,
					  t % 1000);
			}
		}		    
		if (ntohs(slp->csl_rel) != 0xffff) {
		    printf(" rel 0x%4x");
		}
		break;

	default:
		printf("slarp-%d",
		       ntohl(slp->csl_code));
		break;
	}
}

static void
chdlc_cdp_printf(const char *type, const u_char *p, int length)
{
	int cont = 0;
	char buf[BUFSIZ + 4], *dp = buf;
	const u_char *lp;

	if (qflag && length > 8) {
		lp = p + 8;
		cont++;
	} else {
		lp = p + length;
	}

	for (p; p < lp; p++) {
		char c = *p;

		if (isprint(c)) {
			*dp++ = c;
		} else if (iscntrl(c)) {
			switch (c) {
			case '\n':
				*dp++ = '\\';
				*dp++ = 'n';
				break;

			case '\r':
				*dp++ = '\\';
				*dp++ = 'r';
				break;

			case '\t':
				*dp++ = '\\';
				*dp++ = 'r';
				break;

			default:
				goto octal;
			}
		} else {
 octal:
			dp += sprintf(dp, "\\%o", c);
		}
		if (dp > buf + BUFSIZ) {
			cont++;
			break;
		}
	}
	*dp = (char) 0;

	printf(" {%s %s%s}",
	       type,
	       buf,
	       cont ? "..." : "");
}


void
chdlc_cdp_print(const u_char *p, int length)
{
	const u_char *lp = min(p + length, snapend);
	int type, len;

	/* Skip unknown header */
	p += 4 ;

	for (; p < lp; p += len) {
		if (p + 4 > lp
		    || p + (len = EXTRACT_16BITS(p + 2)) > lp) {
			(void) printf(" [|cdp]");
			return;
		}
		switch ((type = EXTRACT_16BITS(p))) {
		case 1:	/* Host name */
			chdlc_cdp_printf("host", p + 4, len - 4);
			break;

		case 3: /* Interface name */
			chdlc_cdp_printf("intf", p + 4, len - 4);
			break;

		case 5: /* Software ID */
			chdlc_cdp_printf("sw", p + 4, len - 4);
			break;

		case 6: /* Hardware ID */
			chdlc_cdp_printf("hw", p + 4, len - 4);
			break;

		default:
			if (vflag) {
				printf(" {type-%u[%u]}",
				       type,
				       len);
			}
			break;
		}
	}
}

static void
chdlc_print(const struct cisco_hdr *chp, int length)
{
	/* The direction is stored in the control field */
	printf("%c ",
	       (chp->csh_ctl == CISCO_DIR_OUT) ? 'O' : 'I');
		       
	switch (chp->csh_addr) {
	case CISCO_ADDR_UNICAST:
		printf("unicast ");
		break;

	case CISCO_ADDR_BCAST:
		printf("bcast ");
		break;

	default:
		printf("chdlc address-%d ",
		       chp->csh_addr);
		break;
	}

#ifdef	notdef
	/* Control byte is used for direction */
	if (chp->csh_ctl) {
		printf("control: %x ",
		       chp->csh_ctl);
	}
#endif

	switch (ntohs(chp->csh_type)) {
	case CISCO_TYPE_SLARP:
		printf("slarp ");
		break;

#ifdef	CISCO_TYPE_CDP
	case CISCO_TYPE_CDP:
		printf("cdp ");
		break;
#endif	/* CISCO_TYPE_CDP */

	default:
		printf("%s ",
		       etherproto_string(chp->csh_type));
		break;
	}

	printf ("%d: ", length);
}

void
chdlc_if_print(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{
	int length = h->len;
	int caplen = h->caplen;
	const u_char *pp;
	const struct cisco_hdr *chp;
	const struct cisco_slarp *slp;
	char *direction = "";

	ts_print(&h->ts);

	if (caplen < CHDLC_HDRLEN) {
		printf("[|cisco_hdlc]");
		goto out;
	}
	    
	/*
	 * Some printers want to get back at the link level addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	packetp = p;
	snapend = p + caplen;

	chp = (struct cisco_hdr *) p;
	if (eflag)
		chdlc_print(chp, length);
	else
		direction = (chp->csh_ctl == CISCO_DIR_OUT) ? "O " : "I ";

	length -= CHDLC_HDRLEN;
	caplen -= CHDLC_HDRLEN;
	pp = p + CHDLC_HDRLEN;

	switch (ntohs(chp->csh_type)) {
	case ETHERTYPE_IP:
		ip_print(pp, length);
		break;

#ifdef	INET6
	case ETHERTYPE_IPV6:
		ipv6_print(pp, length);
		break;
#endif

#ifdef	CISCO_TYPE_CDP
	case CISCO_TYPE_CDP:
		if (!eflag)
			printf("%scdp:",
			       direction);
		chdlc_cdp_print(pp, length);
		break;
#endif	/* CISCO_TYPE_CDP */

	case ETHERTYPE_REVARP:
		if (!eflag)
			printf("%sslarp:",
			       direction);
		chdlc_slarp_print(pp, caplen);
		break;

	case ETHERTYPE_DN:
		decnet_print(pp, length, caplen);
		break;

	case ETHERTYPE_ATALK:
		atalk_print(pp, length);
		break;

	case ETHERTYPE_AARP:
		aarp_print(pp, length);
		break;

	default:
		if (!eflag) {
			printf("type %04x: ",
			       ntohs(chp->csh_type));
		}
		break;
	}

	if (xflag)
		default_print(pp, caplen);

    out:
	if (xxflag)
		hexdump(packetp, h->caplen);
	putchar('\n');
}
#else
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>

#include "interface.h"
void
chdlc_if_print(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{
	error("not configured for cisco hdlc");
	/* NOTREACHED */
}
#endif

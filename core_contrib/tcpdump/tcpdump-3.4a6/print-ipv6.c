/*
%%% portions-copyright-cmetz-98
Portions of this software are Copyright 1998 by Craig Metz, All Rights
Reserved. The Inner Net License Version 2 applies to these portions of
the software.
You should have received a copy of the license with this software. If
you didn't get a copy, you may request one from <license@inner.net>.

*/
/* TCPDUMP 4 IPv6 */
/*
 * print-ipv6.c  --  Prints IPv6 information for tcpdump.
 *
 * Originally from 4.4-Lite BSD.  Modifications to support IPv6 and IPsec
 * are copyright 1995 by Ken Chin, Bao Phan, Randall Atkinson, & Dan McDonald,
 * All Rights Reserved.  All Rights under this copyright are assigned to NRL.
 ----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
#	@(#)COPYRIGHT	1.1 (NRL) 17 January 1995

COPYRIGHT NOTICE

All of the documentation and software included in this software
distribution from the US Naval Research Laboratory (NRL) are
copyrighted by their respective developers.

Portions of the software are derived from the Net/2 and 4.4 Berkeley
Software Distributions (BSD) of the University of California at
Berkeley and those portions are copyright by The Regents of the
University of California. All Rights Reserved.  The UC Berkeley
Copyright and License agreement is binding on those portions of the
software.  In all cases, the NRL developers have retained the original
UC Berkeley copyright and license notices in the respective files in
accordance with the UC Berkeley copyrights and license.

Portions of this software and documentation were developed at NRL by
various people.  Those developers have each copyrighted the portions
that they developed at NRL and have assigned All Rights for those
portions to NRL.  Outside the USA, NRL has copyright on some of the
software developed at NRL. The affected files all contain specific
copyright notices and those notices must be retained in any derived
work.

NRL LICENSE

NRL grants permission for redistribution and use in source and binary
forms, with or without modification, of the software and documentation
created at NRL provided that the following conditions are met:

1. All terms of the UC Berkeley copyright and license must be followed.
2. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
3. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
4. All advertising materials mentioning features or use of this software
   must display the following acknowledgements:

	This product includes software developed by the University of
	California, Berkeley and its contributors.

	This product includes software developed at the Information
	Technology Division, US Naval Research Laboratory.

5. Neither the name of the NRL nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THE SOFTWARE PROVIDED BY NRL IS PROVIDED BY NRL AND CONTRIBUTORS ``AS
IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL NRL OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation
are those of the authors and should not be interpreted as representing
official policies, either expressed or implied, of the US Naval
Research Laboratory (NRL).

----------------------------------------------------------------------*/

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992, 1993, 1994
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

#include <sys/param.h>

#if INET6
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>

#include <netdb.h>

#include <netinet6/ipv6.h>

#include <stdio.h>
#ifdef __STDC__
#include <stdlib.h>
#endif
#include <unistd.h>

#include "interface.h"
#include "addrtoname.h"

#ifdef IPSEC
#include <netsec/ipsec.h>

void ah_print(void *);
void esp_print(void *);
#endif /* IPSEC */

static void hop_print(const u_char * bp)
{
	printf(" hop");
}

struct __ipv6_fraghdr
{
  u_int8_t frag_nextheader;
  u_int8_t frag_reserved;
  u_int16_t frag_bitsoffset;
  u_int32_t frag_id;
};

struct __ipv6_opthdr
{
  u_int8_t oh_nextheader;
  u_int8_t oh_extlen;
  u_int8_t oh_data[6];
};

struct __ipv6_srcroute0
{
  u_int8_t i6sr_nextheader;
  u_int8_t i6sr_numaddrs;
  u_int8_t i6sr_type;
  u_int8_t i6sr_left;
  u_int8_t i6sr_reserved;
  u_int8_t i6sr_mask[3];
};

static void frag_print(struct __ipv6_fraghdr *ip)
{
  u_int16_t bitoffset;

  printf(" frag[%d", ip->frag_reserved);
  bitoffset = ntohs(ip->frag_bitsoffset);
  printf(" %d", bitoffset & 0xfff8);
  printf(" %d", bitoffset & 0x1);
  printf(" %u] ", ntohl(ip->frag_id));
}

static void route_print(struct __ipv6_srcroute0 *ip)
{
	printf(" %d", ip->i6sr_type);
	if (!&ip->i6sr_type)  {
		printf(" %d", ip->i6sr_numaddrs);
	}
}

static void dest_print(struct __ipv6_opthdr *ip)
{
	printf(" %d", ip->oh_extlen);
}

void ipv6_print (register const u_char *bp, register int length)
{
	char buf[80];
	register const struct ipv6hdr *ip;
	u_int8_t nhdr;
	int first = 1, len=0;
	char *cp;
	u_int32_t p;	
	struct hostent *name;

	ip= (const struct ipv6hdr *) bp;
	if (length < sizeof(struct ipv6hdr)){
	  (void)printf("truncated-ipv6 %d", length);
	  return;
	}
	
	len = sizeof(struct ipv6hdr) + ntohs(ip->ipv6_len);

	if (length < len)
	  (void)printf("truncated-ipv6 - %d bytes missing!",
		       len - length);

	len = sizeof(struct ipv6hdr);

        printf(" %s > %s ", ipv6addr_string(&(ip->ipv6_src)), 
	       ipv6addr_string(&(ip->ipv6_dst)));

	if (vflag) {
	  printf(" (v%d,", ip->ipv6_version);                     /* version */
	  printf(" priority %d,", (p&0x0f000000)>>24); /* priority */
	  printf(" flow %d,", p&0x00ffffff);	       /* flow label */
	  printf(" len %d,", ntohs(ip->ipv6_len));  /* payload length */
	  printf(" hop %d) ", ip->ipv6_hoplimit);
	}
	
	nhdr = ip->ipv6_nextheader;
	cp = (char *)(ip + 1);

	while ((nhdr!=IPPROTO_NONE)&&(nhdr!=IPPROTO_TCP)&&(nhdr!=IPPROTO_UDP)
	       &&(nhdr!=IPPROTO_ICMPV6)&&(nhdr!=IPPROTO_IPV4)&&
	       (nhdr!=IPPROTO_IPV6))
		switch (nhdr) {

		case IPPROTO_HOPOPTS:	/* hop by hop (0)*/
			len += (((struct ipv6_opthdr *)cp)->oh_extlen << 4);
			if (len<snaplen)
				hop_print(cp);
			else
				printf(" hop-hdr");
			nhdr = ((struct ipv6_opthdr *)cp)->oh_nexthdr;
			cp += (((struct ipv6_opthdr *)cp)->oh_extlen << 4);
			break;

		case IPPROTO_ROUTING:	/* routing header (43)*/
			len += sizeof(struct __ipv6_srcroute0 );
			if ((len<snaplen)&&(vflag))
				route_print((struct __ipv6_srcroute0 *)cp);
			else
				printf(" route-hdr");
			nhdr = ((struct __ipv6_srcroute0 *)cp)->i6sr_nextheader;
			break;

		case IPPROTO_FRAGMENT:	/* fragment header (44)*/
			len += sizeof(struct __ipv6_fraghdr);
			if ((len<snaplen)&&(vflag))
				frag_print((struct __ipv6_fraghdr *)cp);
			else
				printf(" frag ");
			nhdr = ((struct __ipv6_fraghdr *)cp)->frag_nextheader;
			printf("nhdr=");
			switch (nhdr)  {
			case IPPROTO_HOPOPTS:
				printf("hop");
				break;
			case IPPROTO_IGMP:
				printf("igmp");
				break;
			case IPPROTO_TCP:
				printf("tcp");
				break;
			case IPPROTO_UDP:
				printf("udp");
				break;
			case IPPROTO_IPV6:
				printf("ipv6");
				break;
			case IPPROTO_ROUTING:
				printf("routing");
				break;
			case IPPROTO_FRAGMENT:
				printf("frag");
				break;
#ifdef IPSEC
			case IPPROTO_ESP:
				printf("esp");
				break;
			case IPPROTO_AH:
				printf("auth");
				break;
#endif /* IPSEC */
			case IPPROTO_ICMPV6:
				printf("icmpv6");
				break;
			case IPPROTO_NONE: 
				printf("no next");
				break;
			case IPPROTO_DSTOPTS:
				printf("dest");
				break;
			default:
				printf("%d", nhdr);
			}
			return;
#ifdef IPSEC
		case IPPROTO_ESP:	/* encapsulation header (50)*/
			len += sizeof(u_int32_t);
			if ((len<snaplen)&&(vflag)) {
				esp_print((char *)cp);
				printf(" ");
			} else
				printf(" esp");
			return;

		case IPPROTO_AH:	/* authentication header (51)*/
			len += sizeof(struct ipsec_ah) + sizeof(u_int32_t) * ((struct ipsec_ah *)cp)->ah_datalen;
			if ((len<snaplen)&&(vflag)) {
				ah_print((struct ipsec_ah *)cp);
				printf(" ");
			} else
				printf(" ah");
			nhdr = ((struct ipsec_ah *)cp)->ah_nexthdr;
			cp += ((struct ipsec_ah *)cp)->ah_datalen * sizeof (u_int32_t)+ sizeof(struct ipsec_ah);
			break;
#endif /* IPSEC */

		case IPPROTO_DSTOPTS:	/* destination options header (60)*/
			len += sizeof(struct __ipv6_opthdr);
			if ((len<snaplen)&&(vflag))
				dest_print((struct __ipv6_opthdr *)cp);
			else
				printf(" dest-hdr");
			nhdr = ((struct __ipv6_opthdr *)cp)->oh_nextheader;
			cp += sizeof(struct __ipv6_opthdr);
			break;

		default:
			printf("unknown nexthdr %d", nhdr);
			return;

	      }
	len = length - len;
	switch (nhdr) {
		case IPPROTO_TCP:
			tcp_print(cp, len, (const u_char *)ip);
                        break; 
		case IPPROTO_UDP:
			udp_print(cp, len, (const u_char *)ip);
                        break; 
		case IPPROTO_ICMPV6:
			icmpv6_print(cp, len);
                        break;
	        case IPPROTO_IPV4:
			printf(": v4-in-v6");
			printf("\n                ");
			ip_print(cp, len);
			break;
	        case IPPROTO_IPV6:
			printf(": v6-in-v6");
			printf("\n                ");
			ipv6_print(cp, len);
			break;
		default:
			printf("unknown transport protocol");
			return;
	}
}
#endif /* INET6 */

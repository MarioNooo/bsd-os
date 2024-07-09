/*
%%% portions-copyright-cmetz-98
Portions of this software are Copyright 1998 by Craig Metz, All Rights
Reserved. The Inner Net License Version 2 applies to these portions of
the software.
You should have received a copy of the license with this software. If
you didn't get a copy, you may request one from <license@inner.net>.

*/
/*
 * print-icmpv6.c  --  Display ICMPv6 data.
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
 * Copyright (c) 1988, 1989, 1990, 1991, 1993, 1994
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

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>

#include <netinet6/ipv6.h>
#include <netinet6/icmpv6.h>
#include <netinet6/nd6_protocol.h>

#include <stdio.h>

#include <string.h>

#include "interface.h"
#include "addrtoname.h"

char *ipv6addr_string(struct in6_addr *);

struct nbr_ad {
  u_char type;
  u_char code;
  u_int16_t chksum;
  u_int32_t flags;
  struct in6_addr targ_addr;
};

struct nbr_sol
{
  u_char type;
  u_char code;
  u_int16_t cksum;
  u_int32_t reserved;
  struct in6_addr target;
};

struct na_opt {
  u_char type;
  u_char length;
  u_char data[6];
};
		
void na_opt_print(struct na_opt *na, char *s)
{
	char *addr;
	int i;
	char buf[256];

	switch(na->type)  {
	case 1:
		s = strcat(s, " src[");		
		break;
	case 2:
		s = strcat(s, " target[");
		break;
	}
	for (i=0; i< (na->length * 8)-2; i++) {
		snprintf(buf, sizeof(buf), "%x", na->data[i]);
		if ( i == (na->length*8)-3 )  { 
			s = strcat(s,(char *)buf);
			s = strcat(s, "]");
		}  else  {
			s = strcat(s,(char*)buf);
			s = strcat(s,":");
		}
	}
}

void
icmpv6_print(register const u_char *bp, int length)
{
	register const struct icmpv6hdr *icmpv6;
	register char *str;
	register const struct ipv6 *oip;
	register const struct uicmpv6hdr *ouh;
	char buf[256];
	char src[80];
	char dst[80];
	char *cp;
	struct nbr_ad *na;
	struct nbr_sol *ns;
	struct na_opt *opt;
	char *targ_addr;
	int na_len, left;

	icmpv6 = (struct icmpv6hdr *)bp;
	left = (u_int)snapend - (u_int)bp;

	str = buf;
	*str = '\0';

	if (left < sizeof(struct icmpv6hdr))
	  goto trunc;

	switch (icmpv6->icmpv6_type) {
	case ICMPV6_ECHO_REPLY:
		str = "echo reply";
		break;
	case ICMPV6_DST_UNREACH:
		switch (icmpv6->icmpv6_code) {
		case ICMPV6_UNREACH_NOROUTE:
			str = "no route";
			break;
		case ICMPV6_UNREACH_ADMIN:
			str = "admin prohibited";
			break;
		case ICMPV6_UNREACH_NOTNEIGHBOR:
			str ="not a neighbor";
			break;
		case ICMPV6_UNREACH_ADDRESS:
			str = "host unreachable";
 			break;
/*		case ICMPV6_UNREACH_PROTOCOL:
			str = "protocol unreachable";
			break;*/
		case ICMPV6_UNREACH_PORT:
			str = "port unreachable";
			break;
		}
		break;
	case ICMPV6_PACKET_TOOBIG:
		sprintf(str, "packet too big: mtu[%d]", ntohl(icmpv6->icmpv6_mtu));
		break;
	case ICMPV6_ECHO_REQUEST:
		str = "echo request";
		break;
	case ICMPV6_MEMBERSHIP_QUERY:
		str = "query group membership";
		break;
	case ICMPV6_MEMBERSHIP_REPORT:
		str = "join mcast group";
		break;
	case ICMPV6_MEMBERSHIP_REDUCTION:
		str = "leave mcast group";
		break;
	case ICMPV6_TIME_EXCEEDED:
		switch (icmpv6->icmpv6_code) {
		case ICMPV6_EXCEEDED_HOPS:
			str = "hop limit exceeded";
			break;
		case ICMPV6_EXCEEDED_REASSEMBLY:
			str = "reassembly time exceeded";
			break;
		}
		break;
	case ICMPV6_PARAMETER_PROBLEM:
		switch(icmpv6->icmpv6_code) {
		case ICMPV6_PARAMPROB_HDR:
		  strcpy(str, "bad header field");
		  break;
		case ICMPV6_PARAMPROB_NEXTHDR:
		  strcpy(str,"bad next header");
		  break;
		case ICMPV6_PARAMPROB_OPTION:
		  strcpy(str,"unrec. option");
		  break;
		}
		sprintf(str+strlen(str), " ptr[%x]", icmpv6->icmpv6_pptr);
		break;
#ifndef ICMPV6_ROUTER_SOLICITATION
#define ICMPV6_ROUTER_SOLICITATION ND6_ROUTER_SOLICITATION
#endif /* ICMPV6_ROUTER_SOLICITATION */
	case ICMPV6_ROUTER_SOLICITATION:
		str = "router solicit";
		break;
#ifndef ICMPV6_ROUTER_ADVERTISEMENT
#define ICMPV6_ROUTER_ADVERTISEMENT ND6_ROUTER_ADVERTISEMENT
#endif /* ICMPV6_ROUTER_ADVERTISEMENT */
	case ICMPV6_ROUTER_ADVERTISEMENT:
		str = "router ad";
		break;
#ifndef ICMPV6_NEIGHBOR_SOLICITATION
#define ICMPV6_NEIGHBOR_SOLICITATION ND6_NEIGHBOR_SOLICITATION
#endif /* ICMPV6_NEIGHBOR_SOLICITATION */
	case ICMPV6_NEIGHBOR_SOLICITATION:
		if (left < sizeof(struct nbr_sol))
		  goto trunc;
                str = strcat(str, "neigh sol ");
		ns = (struct nbr_sol *)bp;
                na_len = sizeof (struct nbr_sol);
		if (vflag)
		  {
		    str = strcat(str, "for ");
                    str = strcat(str, ipv6addr_string(&(ns->target)));
		  }
                ns++;   
                cp = (char *)ns;
                opt = (struct na_opt *)ns;
		if (vflag)
		  while (((na_len + opt->length * 8) <= left) && opt->length) {
                        na_opt_print((struct na_opt *)opt, str);
                        cp += opt->length;
                        na_len += (opt->length * 8);
                        opt = (struct na_opt *)cp;
		      }
		break;
#ifndef ICMPV6_NEIGHBOR_ADVERTISEMENT
#define ICMPV6_NEIGHBOR_ADVERTISEMENT ND6_NEIGHBOR_ADVERTISEMENT
#endif /* ICMPV6_NEIGHBOR_ADVERTISEMENT */
	case ICMPV6_NEIGHBOR_ADVERTISEMENT:
		if (left < sizeof(struct nbr_ad))
		  goto trunc;
		str = strcat(str, "neigh adv ");
		na = (struct nbr_ad *)bp;
		na_len = sizeof (struct nbr_ad);
		str = strcat(str, "(");
		/*
		 * Flags are in network order, but so are bitmasks.
		 */
		if (na->flags & ND6_NADVERFLAG_ISROUTER)
		  str = strcat(str, "R");
		if (na->flags & ND6_NADVERFLAG_SOLICITED)
		  str = strcat(str, "S");
		if (na->flags & ND6_NADVERFLAG_OVERRIDE)
		  str = strcat(str, "O");
#if 0
		if (na->flags & ICMPV6_NEIGHBORADV_RTR)
		  str = strcat(str, "R");
		if (na->flags & ICMPV6_NEIGHBORADV_SOL)
		  str = strcat(str, "S");
		if (na->flags & ICMPV6_NEIGHBORADV_OVERRIDE)
		  str = strcat(str, "O");
		if (na->flags & ICMPV6_NEIGHBORADV_SECOND)
		  str = strcat(str, "N");
#endif /* 0 */
		str = strcat(str, ") ");
		if (vflag)  {
		  str = strcat(str, "for ");
                  str = strcat(str, ipv6addr_string(&(na->targ_addr)));
		}
		na++;
		cp = (char *)na; 
		opt = (struct na_opt *)na;
		if (vflag)
		while (((na_len + opt->length*8) <= left) && opt->length)  {
			na_opt_print((struct na_opt *)opt, str);
			cp += opt->length;
			na_len += (opt->length * 8);
			opt = (struct na_opt *)cp;
		}
		break;
	default:
		(void)snprintf(buf, sizeof(buf), "type#%d", icmpv6->icmpv6_type);
		break;
	}
        (void)printf("icmpv6: %s", str);
	return;
trunc:
	fputs("[|icmpv6]", stdout);
#undef TCHECK
}

#endif /* INET6 */

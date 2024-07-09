/*	BSDI inet.c,v 2.8 1996/11/12 20:08:05 jch Exp	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)inet.c	8.4 (Berkeley) 4/20/94";
#endif /* not lint */

/*
 * We need the normally kernel-only struct selinfo definition
 */
#define _NEED_STRUCT_SELINFO

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>

#include <net/route.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/in_systm.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/pim6_var.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "netstat.h"

#ifdef notyet
static int inet6stat_mib[] = { CTL_NET, PF_INET6, IPPROTO_IP, IPV6CTL_STATS };
#endif 
static struct ip6stat ip6stat;
struct data_info inet6stat_info = {
	"_ip6stat",
#ifdef notyet
	inet6stat_mib, sizeof(inet6stat_mib)/sizeof(*inet6stat_mib),
#else
	NULL, 0,
#endif 
	&ip6stat, sizeof(ip6stat)
};
static int icmp6stat_mib[] = { CTL_NET, PF_INET6, IPPROTO_ICMPV6,
			       ICMPV6CTL_STATS };
static struct icmp6stat icmp6stat;
struct data_info icmp6stat_info = {
	"_icmp6stat",
	icmp6stat_mib, sizeof(icmp6stat_mib)/sizeof(*icmp6stat_mib),
	&icmp6stat, sizeof(icmp6stat)
};

char	*inet6name __P((struct in6_addr *));
void	inet6print __P((struct in6_addr *, int, char *));

/*
 * Dump IPv6 per-interface statistics based on RFC 2465.
 */
void
ip6_ifstats(ifname)
	char *ifname;
{
	struct in6_ifreq ifr;
	int s;
#define	p(f, m) if (ifr.ifr_ifru.ifru_stat.f || sflag <= 1) \
    printf(m, (u_quad_t)ifr.ifr_ifru.ifru_stat.f, PLURAL(ifr.ifr_ifru.ifru_stat.f))
#define	p_5(f, m) if (ifr.ifr_ifru.ifru_stat.f || sflag <= 1) \
    printf(m, (u_quad_t)ip6stat.f)

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		perror("Warning: socket(AF_INET6)");
		return;
	}

	strcpy(ifr.ifr_name, ifname);
	printf("ip6 on %s:\n", ifr.ifr_name);

	if (ioctl(s, SIOCGIFSTAT_IN6, (char *)&ifr) < 0) {
		perror("Warning: ioctl(SIOCGIFSTAT_IN6)");
		goto end;
	}

	p(ifs6_in_receive, "\t%qu total input datagram%s\n");
	p(ifs6_in_hdrerr, "\t%qu datagram%s with invalid header received\n");
	p(ifs6_in_toobig, "\t%qu datagram%s exceeded MTU received\n");
	p(ifs6_in_noroute, "\t%qu datagram%s with no route received\n");
	p(ifs6_in_addrerr, "\t%qu datagram%s with invalid dst received\n");
	p(ifs6_in_protounknown, "\t%qu datagram%s with unknown proto received\n");
	p(ifs6_in_truncated, "\t%qu truncated datagram%s received\n");
	p(ifs6_in_discard, "\t%qu input datagram%s discarded\n");
	p(ifs6_in_deliver,
	  "\t%qu datagram%s delivered to an upper layer protocol\n");
	p(ifs6_out_forward, "\t%qu datagram%s forwarded to this interface\n");
	p(ifs6_out_request,
	  "\t%qu datagram%s sent from an upper layer protocol\n");
	p(ifs6_out_discard, "\t%qu total discarded output datagram%s\n");
	p(ifs6_out_fragok, "\t%qu output datagram%s fragmented\n");
	p(ifs6_out_fragfail, "\t%qu output datagram%s failed on fragment\n");
	p(ifs6_out_fragcreat, "\t%qu output datagram%s succeeded on fragment\n");
	p(ifs6_reass_reqd, "\t%qu incoming datagram%s fragmented\n");
	p(ifs6_reass_ok, "\t%qu datagram%s reassembled\n");
	p(ifs6_reass_fail, "\t%qu datagram%s failed on reassembling\n");
	p(ifs6_in_mcast, "\t%qu multicast datagram%s received\n");
	p(ifs6_out_mcast, "\t%qu multicast datagram%s sent\n");

  end:
	close(s);

#undef p
#undef p_5
}

/*
 * Dump ICMPv6 per-interface statistics based on RFC 2466.
 */
void
icmp6_ifstats(ifname)
	char *ifname;
{
	struct in6_ifreq ifr;
	int s;
#define	p(f, m) if (ifr.ifr_ifru.ifru_icmp6stat.f || sflag <= 1) \
    printf(m, (u_quad_t)ifr.ifr_ifru.ifru_icmp6stat.f, PLURAL(ifr.ifr_ifru.ifru_icmp6stat.f))

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		perror("Warning: socket(AF_INET6)");
		return;
	}

	strcpy(ifr.ifr_name, ifname);
	printf("icmp6 on %s:\n", ifr.ifr_name);

	if (ioctl(s, SIOCGIFSTAT_ICMP6, (char *)&ifr) < 0) {
		perror("Warning: ioctl(SIOCGIFSTAT_ICMP6)");
		goto end;
	}

	p(ifs6_in_msg, "\t%qu total input message%s\n");
	p(ifs6_in_error, "\t%qu total input error message%s\n"); 
	p(ifs6_in_dstunreach, "\t%qu input destination unreachable error%s\n");
	p(ifs6_in_adminprohib, "\t%qu input administratively prohibited error%s\n");
	p(ifs6_in_timeexceed, "\t%qu input time exceeded error%s\n");
	p(ifs6_in_paramprob, "\t%qu input parameter problem error%s\n");
	p(ifs6_in_pkttoobig, "\t%qu input packet too big error%s\n");
	p(ifs6_in_echo, "\t%qu input echo request%s\n");
	p(ifs6_in_echoreply, "\t%qu input echo reply%s\n");
	p(ifs6_in_routersolicit, "\t%qu input router solicitation%s\n");
	p(ifs6_in_routeradvert, "\t%qu input router advertisement%s\n");
	p(ifs6_in_neighborsolicit, "\t%qu input neighbor solicitation%s\n");
	p(ifs6_in_neighboradvert, "\t%qu input neighbor advertisement%s\n");
	p(ifs6_in_redirect, "\t%qu input redirect%s\n");
	p(ifs6_in_mldquery, "\t%qu input MLD query%s\n");
	p(ifs6_in_mldreport, "\t%qu input MLD report%s\n");
	p(ifs6_in_mlddone, "\t%qu input MLD done%s\n");

	p(ifs6_out_msg, "\t%qu total output message%s\n");
	p(ifs6_out_error, "\t%qu total output error message%s\n");
	p(ifs6_out_dstunreach, "\t%qu output destination unreachable error%s\n");
	p(ifs6_out_adminprohib, "\t%qu output administratively prohibited error%s\n");
	p(ifs6_out_timeexceed, "\t%qu output time exceeded error%s\n");
	p(ifs6_out_paramprob, "\t%qu output parameter problem error%s\n");
	p(ifs6_out_pkttoobig, "\t%qu output packet too big error%s\n");
	p(ifs6_out_echo, "\t%qu output echo request%s\n");
	p(ifs6_out_echoreply, "\t%qu output echo reply%s\n");
	p(ifs6_out_routersolicit, "\t%qu output router solicitation%s\n");
	p(ifs6_out_routeradvert, "\t%qu output router advertisement%s\n");
	p(ifs6_out_neighborsolicit, "\t%qu output neighbor solicitation%s\n");
	p(ifs6_out_neighboradvert, "\t%qu output neighbor advertisement%s\n");
	p(ifs6_out_redirect, "\t%qu output redirect%s\n");
	p(ifs6_out_mldquery, "\t%qu output MLD query%s\n");
	p(ifs6_out_mldreport, "\t%qu output MLD report%s\n");
	p(ifs6_out_mlddone, "\t%qu output MLD done%s\n");

  end:
	close(s);
#undef p
}

/*
 * Dump PIM statistics structure.
 */
void
pim6_stats(off, name)
	u_long off;
	char *name;
{
	struct pim6stat pim6stat;

	if (off == 0)
		return;
	kread(off, (char *)&pim6stat, sizeof(pim6stat));
	printf("%s:\n", name);

#define	p(f, m) if (pim6stat.f || sflag <= 1) \
    printf(m, (u_quad_t)pim6stat.f, PLURAL(pim6stat.f))
	p(pim6s_rcv_total, "\t%qu message%s received\n");
	p(pim6s_rcv_tooshort, "\t%qu message%s received with too few bytes\n");
	p(pim6s_rcv_badsum, "\t%qu message%s received with bad checksum\n");
	p(pim6s_rcv_badversion, "\t%qu message%s received with bad version\n");
	p(pim6s_rcv_registers, "\t%qu register%s received\n");
	p(pim6s_rcv_badregisters, "\t%qu bad register%s received\n");
	p(pim6s_snd_registers, "\t%qu register%s sent\n");
#undef p
}

#define GETSERVBYPORT6(port, proto, ret)\
do {\
	if (strcmp((proto), "tcp6") == 0)\
		(ret) = getservbyport((int)(port), "tcp");\
	else if (strcmp((proto), "udp6") == 0)\
		(ret) = getservbyport((int)(port), "udp");\
	else\
		(ret) = getservbyport((int)(port), (proto));\
} while (0)

/*
 * Pretty print an Internet address (net address + port).
 * If the nflag was specified, use numbers instead of names.
 */
void
inet6print(in6, port, proto)
	struct in6_addr *in6;
	int port;
	char *proto;
{
	struct servent *sp = 0;
	char line[80], *cp;
	int width;

	sprintf(line, "%.*s.", (Aflag && !nflag) ? 12 : 16, inet6name(in6));
	cp = index(line, '\0');
	if (!nflag && port)
		GETSERVBYPORT6(port, proto, sp);
	if (sp || port == 0)
		sprintf(cp, "%.8s", sp ? sp->s_name : "*");
	else
		sprintf(cp, "%d", ntohs((u_short)port));
	width = Aflag ? 18 : 22;
	printf(" %-*.*s", width, width, line);
}

/*
 * Construct an Internet address representation.
 * If the nflag has been supplied, give
 * numeric value, otherwise try for symbolic name.
 */
char *
inet6name(in6p)
	struct in6_addr *in6p;
{
	char *cp;
	static char line[50];
	struct hostent *hp;
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1;
	char ntop_buf[INET6_ADDRSTRLEN];


	if (first && !nflag) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = index(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	cp = 0;
	if (!nflag && IN6_IS_ADDR_UNSPECIFIED(in6p)) {
		hp = gethostbyaddr((char *)in6p, sizeof (*in6p), AF_INET);
		if (hp) {
			if ((cp = index(hp->h_name, '.')) &&
			    !strcmp(cp + 1, domain))
				*cp = 0;
			cp = hp->h_name;
		}
	}
	if (IN6_IS_ADDR_UNSPECIFIED(in6p))
		strcpy(line, "*");
	else if (cp)
		strcpy(line, cp);
	else {
		sprintf(line, "%s", inet_ntop(AF_INET6, (void *)in6p, ntop_buf,
			sizeof(ntop_buf)));
	}
	return (line);
}

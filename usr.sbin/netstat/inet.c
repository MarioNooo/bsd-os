/*	BSDI inet.c,v 2.18 1999/11/15 16:15:16 jch Exp	*/
/*----------------------------------------------------------------------
 inet.c:    netstat subroutines for AF_INET/AF_INET6.

 Originally from 4.4-Lite BSD.  Modifications to support IPv6 and IPsec
 copyright by Bao Phan, Randall Atkinson, & Dan McDonald, All Rights Reserved.
 All rights under this copyright are assigned to NRL.
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
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>

#include <net/if.h>	/* for IFNAMSIZ */

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/igmp_var.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_seq.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_debug.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <netinet/icmp6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/ipsec.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "netstat.h"
#include "stats.h"

#define	FMT(s)	"\t" s "\n"
#define	FMT_I(s)	"\t\t" s "\n"
#define	FMT_Q	"%qu"

static u_quad_t udps_delivered;
static u_quad_t icmps_sent, icmps_received;

#include "stats_inet.h"

static u_quad_t icmp6s_sent, icmp6s_received;
#include "stats_inet6.h"

struct	inpcb inpcb;
struct	tcpcb tcpcb;
struct	socket sockb;

struct data_info tcb_info = {
	"_tcb",
	NULL, 0,
	NULL, 0
};

struct data_info udb_info = {
	"_udb",
	NULL, 0,
	NULL, 0
};

/*
 * Print a summary of connections related to an Internet
 * protocol.  For TCP, also give state of connection.
 * Listening processes (aflag) are suppressed unless the
 * -a (all) flag is specified.
 */
void
inetprotopr(char *name, struct data_info *dip)
{
	struct inpcb cb;
	struct inpcb *prev, *next;
	int istcp;
	static int first = 1;
	int v6 = 0;

	istcp = strcmp(name, "tcp") == 0;

	dip->di_ptr = &cb;
	dip->di_size = sizeof(cb);
	if (skread(name, dip))
		return;

	inpcb = cb;
	prev = (struct inpcb *)dip->di_off;
	if (inpcb.inp_next == (struct inpcb *)dip->di_off)
		return;
	while (inpcb.inp_next != (struct inpcb *)dip->di_off) {
		next = inpcb.inp_next;
		if (kread((u_long)next, &inpcb, sizeof (inpcb)))
			return;
		if (inpcb.inp_prev != prev) {
			printf("???\n");
			break;
		}
		v6 = (inpcb.inp_flags & INP_IPV6) && 
		  !(inpcb.inp_flags & INP_IPV6_MAPPED);

		if (!aflag && ((v6 && IN6_IS_ADDR_UNSPECIFIED(&inpcb.inp_laddr6)) ||
		    (!v6 && (inpcb.inp_laddr.s_addr == INADDR_ANY)))) {
			prev = next;
			continue;
		}
		if (kread((u_long)inpcb.inp_socket, &sockb, sizeof (sockb)))
			return;
		if (istcp &&
		    kread((u_long)inpcb.inp_ppcb, &tcpcb, sizeof (tcpcb)))
			return;
		if (first) {
			printf("Active Internet connections");
			if (aflag)
				printf(" (including servers)");
			putchar('\n');
			if (Aflag)
				printf("%-8.8s ", "PCB");
			printf(Aflag ?
				"%-6.6s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n" :
				"%-6.6s %-6.6s %-6.6s  %-22.22s %-22.22s %s\n",
				"Proto", "Recv-Q", "Send-Q",
				"Local Address", "Foreign Address", "(state)");
			first = 0;
		}
		if (Aflag) {
			if (istcp)
				printf("%8lx ", (u_long)inpcb.inp_ppcb);
			else
				printf("%8lx ", (u_long)next);
		}
		if (v6) {
			printf("%-3.3s6   %6ld %6ld ", name, sockb.so_rcv.sb_cc,
			    sockb.so_snd.sb_cc);
		} else
			printf("%-5.5s  %6ld %6ld ", name, sockb.so_rcv.sb_cc,
			    sockb.so_snd.sb_cc);
		if (v6) {
			inetprint(AF_INET6, (struct in_addr *)&inpcb.inp_laddr6,
			    (int)inpcb.inp_lport, name);
			inetprint(AF_INET6, (struct in_addr *)&inpcb.inp_faddr6,
			    (int)inpcb.inp_fport, name);
		} else {
			inetprint(AF_INET, &inpcb.inp_laddr,
			    (int)inpcb.inp_lport, name);
			inetprint(AF_INET, &inpcb.inp_faddr,
			    (int)inpcb.inp_fport, name);
		}
		if (istcp) {
			if (tcpcb.t_state < 0 || tcpcb.t_state >= TCP_NSTATES)
				printf(" %d", tcpcb.t_state);
			else
				printf(" %s", tcpstates[tcpcb.t_state]);
		}
		putchar('\n');
		prev = next;
	}
}


static void
print_stats(const char *name, struct stats *stats, void *ss)
{
	struct stats *sp;

	printf("%s:\n", name);

	for (sp = stats; sp->s_flags != 0; sp++) {
		const char *plural;
		u_quad_t stat1, stat2;

		if (sp->s_flags & S_FMT_DIRECT)
			stat1 = *(u_quad_t *)sp->s_offset[0];
		else
			stat1 = *(u_quad_t *)(ss + (size_t)sp->s_offset[0]);
		switch (S_FMT_TYPE(sp->s_flags)) {
		case S_FMT_5:
			if (stat1 != 0 ||
			    (vflag && !(sp->s_flags & S_FMT_SKIPZERO)))
				(void)printf(sp->s_fmt, stat1);
			break;

		case S_FMT1:
			plural = stat1 == 1 ? "" : "s";
			goto fmt1;

		case S_FMT1ES:
			plural = stat1 == 1 ? "" : "es";
			goto fmt1;

		case S_FMT1IES:
			plural = stat1 == 1 ? "y" : "ies";
			/* Fall through */

		fmt1:
			if (stat1 != 0 ||
			    (vflag && !(sp->s_flags & S_FMT_SKIPZERO)))
				(void)printf(sp->s_fmt, stat1, plural);
			break;

		case S_FMT1_5:
			stat2 = *(u_quad_t *)(ss + (size_t)sp->s_offset[1]);
			if (stat1 != 0 || stat2 != 0 ||
			    (vflag && !(sp->s_flags & S_FMT_SKIPZERO)))
				(void)printf(sp->s_fmt,
				    stat1, stat1 == 1 ? "" : "s",
				    stat2);
			break;

		case S_FMT2:
			stat2 = *(u_quad_t *)(ss + (size_t)sp->s_offset[1]);
			if (stat1 != 0 || stat2 != 0 ||
			    (vflag && !(sp->s_flags & S_FMT_SKIPZERO)))
				(void)printf(sp->s_fmt,
				    stat1, stat1 == 1 ? "" : "s",
				    stat2, stat2 == 1 ? "" : "s");
			break;
		}
	}
}

/*
 * Dump TCP statistics structure.
 */

static struct tcpstat tcpstat;
static int tcpstat_mib[] = { CTL_NET, PF_INET, IPPROTO_TCP, TCPCTL_STATS };
static struct data_info tcpstat_info = {
	"_tcpstat",
	tcpstat_mib, sizeof(tcpstat_mib)/sizeof(*tcpstat_mib),
	&tcpstat, sizeof(tcpstat)
};

void
tcp_stats(char *name)
{

	if (skread(name, &tcpstat_info))
		return;

	print_stats(name, tcp_stat, &tcpstat);
}

/*
 * Dump UDP statistics structure.
 */

static struct udpstat udpstat;
static int udpstat_mib[] = { CTL_NET, PF_INET, IPPROTO_UDP, UDPCTL_STATS };
static struct data_info udpstat_info = {
	"_udpstat",
	udpstat_mib, sizeof(udpstat_mib)/sizeof(*udpstat_mib),
	&udpstat, sizeof(udpstat)
};

void
udp_stats(char *name)
{

	if (skread(name, &udpstat_info))
		return;

	udps_delivered = udpstat.udps_ipackets - udpstat.udps_hdrops -
	    udpstat.udps_badlen - udpstat.udps_badsum - udpstat.udps_noport -
	    udpstat.udps_fullsock;

	print_stats(name, udp_stat, &udpstat);
}

/*
 * Dump IP statistics structure.
 */

static struct ipstat ipstat;
static int ipstat_mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_STATS };
static struct data_info ipstat_info = {
	"_ipstat",
	ipstat_mib, sizeof(ipstat_mib)/sizeof(*ipstat_mib),
	&ipstat, sizeof(ipstat)
};

void
ip_stats(char *name)
{

	if (skread(name, &ipstat_info))
		return;

	print_stats(name, ip_stat, &ipstat);
}

/*
 * Dump IPv6 statistics structure.
 */
static struct ip6stat ip6stat;
static int ip6stat_mib[] = { CTL_NET, PF_INET6, IPPROTO_IPV6, IPV6CTL_STATS };
static struct data_info ip6stat_info = {
	"_ip6stat",
	ip6stat_mib, sizeof(ip6stat_mib)/sizeof(*ip6stat_mib),
	&ip6stat, sizeof(ip6stat)
};

void
ip6_stats(name)
	char *name;
{

	if (skread(name, &ip6stat_info))
		return;

	ip6_stat_init(&ip6stat);
	print_stats(name, ip6_stat, &ip6stat);
}

/*
 * Dump IPSEC statistics structure.
 *
 * Derived from ip6_stats() routine.
 * Includes statistics from ipsec_esp.c (ESP)
 *                      and ipsec_policy.c (In/Out policy checking).
 */

static struct ipsecstat ipsecstats;

static int ipsec_stat_mib[] = { CTL_NET, PF_INET, IPPROTO_AH, IPSECCTL_STATS };
static struct data_info ipsec_stat_info = {
	"_ipsecstat",
	ipsec_stat_mib, sizeof(ipsec_stat_mib)/sizeof(*ipsec_stat_mib),
	&ipsecstats, sizeof(ipsecstats)
};

void
ipsec_stats(name)
	char *name;
{

	if (skread(name, &ipsec_stat_info))
		return;

	ipsec_stat_init(&ipsecstats);
	print_stats(name, ipsec_stat, &ipsecstats);
}

#if 0
/*
 * Dump ESP statistics structure.
 *
 * Derived from ip6_stats() routine.
 * Includes statistics from ipsec_esp.c (ESP)
 *                      and ipsec_policy.c (In/Out policy checking).
 */

static struct esp_stat_table ESPstats;
static int ESP_stat_mib[] = { CTL_NET, PF_INET, IPPROTO_ESP, ESPCTL_STATS };
static struct data_info ESP_stat_info = {
	"_esp_stats",
	ESP_stat_mib, sizeof(ESP_stat_mib)/sizeof(*ESP_stat_mib),
	&ESPstats, sizeof(ESPstats)
};

void
ESP_stats(name)
	char *name;
{

	if (skread(name, &ESP_stat_info))
		return;

	print_stats(name, ESP_stat, &ESPstats);
}

/*
 * Dump AH statistics structure.
 *
 * Derived from ip6_stats() routine.
 * Includes statistics from ipsec_ah.c (AH),
 *                      and ipsec_policy.c (In/Out policy checking).
 */

static struct ah_stat_table AHstats;
static int AH_stat_mib[] = { CTL_NET, PF_INET, IPPROTO_AH, AHCTL_STATS };
static struct data_info AH_stat_info = {
	"_ah_stats",
	AH_stat_mib, sizeof(AH_stat_mib)/sizeof(*AH_stat_mib),
	&AHstats, sizeof(AHstats)
};

void
AH_stats(name)
	char *name;
{

	if (skread(name, &AH_stat_info))
		return;

	print_stats(name, AH_stat, &AHstats);
}
#endif

/*
 * Dump ICMP statistics.
 */

static struct icmpstat icmpstat;
static int icmpstat_mib[] = { CTL_NET, PF_INET, IPPROTO_ICMP, ICMPCTL_STATS };
static struct data_info icmpstat_info = {
	"_icmpstat",
	icmpstat_mib, sizeof(icmpstat_mib)/sizeof(*icmpstat_mib),
	&icmpstat, sizeof(icmpstat)
};

void
icmp_stats(char *name)
{
	int i;

	if (skread(name, &icmpstat_info))
		return;

	icmp_stat_init();
	icmps_received = icmps_sent = 0;
	for (i = 0; i < ICMP_MAXTYPE + 1; i++) {
		icmps_received += icmpstat.icps_inhist[i];
		icmps_sent += icmpstat.icps_outhist[i];
	}

	print_stats(name, icmp_stat, &icmpstat);
}


/*
 * Dump ICMP statistics for IPv6.
 */

static struct icmp6stat icmp6stat;
static int icmp6stat_mib[] = {
	CTL_NET, PF_INET6, IPPROTO_ICMPV6, ICMPV6CTL_STATS
};
static struct data_info icmp6stat_info = {
	"_icmp6stat",
	icmp6stat_mib, sizeof(icmp6stat_mib)/sizeof(*icmp6stat_mib),
	&icmp6stat, sizeof(icmp6stat)
};

void
icmp6_stats(name)
	char *name;
{
	register int i;

	if (skread(name, &icmp6stat_info))
		return;

	icmp6_stat_init();
	icmp6s_received = icmp6s_sent = 0;
	for (i = 0; i < ICMP6_MAXTYPE + 1; i++) {
		icmp6s_received += icmpstat.icps_inhist[i];
		icmp6s_sent += icmpstat.icps_outhist[i];
	}
	print_stats(name, icmp6_stat, &icmp6stat);
}


static struct igmpstat igmpstat;
static int igmpstat_mib[] = { CTL_NET, PF_INET, IPPROTO_IGMP, IGMPCTL_STATS };
static struct data_info igmpstat_info = {
	"_igmpstat",
	igmpstat_mib, sizeof(igmpstat_mib)/sizeof(*igmpstat_mib),
	&igmpstat, sizeof(igmpstat)
};

void
igmp_stats(char *name)
{

	if (skread(name, &igmpstat_info))
		return;

	print_stats(name, igmp_stat, &igmpstat);
}

/*
 * Pretty print an Internet address (net address + port).
 * If the nflag was specified, use numbers instead of names.
 */
void
inetprint(int af, void *addr, int port, char *proto)
{
	union {
		struct sockaddr sa;
		struct sockaddr_in sin;
		struct sockaddr_in6 sin6;
	} su;
	char host[NI_MAXHOST], serv[NI_MAXSERV], line[LINE_MAX], *h, *s;
	int width, niflag;

	h = NULL;
	memset(&su, 0, sizeof(su));
	su.sa.sa_family = af;

	switch (af) {
	case AF_INET:
		su.sa.sa_len = sizeof(struct sockaddr_in);

		if (*(u_int32_t *)addr) {
			memcpy(&su.sin.sin_addr, addr, sizeof(struct in_addr));
			h = host;
		}

		su.sin.sin_port = port;
		break;
	case AF_INET6:
		su.sa.sa_len = sizeof(struct sockaddr_in6);

		if (IN6_IS_ADDR_V4MAPPED((struct in6_addr *)addr)) {
			su.sa.sa_family = AF_INET;
			su.sa.sa_len = sizeof(struct sockaddr_in);
			memcpy(&su.sin.sin_addr,
			    &(((struct in6_addr *)addr)->s6_addr[12]),
			    sizeof(struct in_addr));
			su.sin.sin_port = port;
			h = host;
			break;
		}
		if (memcmp(addr, &in6addr_any, sizeof(struct in6_addr))) {
			memcpy(&su.sin6.sin6_addr, addr, sizeof(struct in6_addr));
			h = host;
		}

		su.sin6.sin6_port = port;
		break;
	}

	if (port)
		s = serv;
	else
		s = NULL;

	niflag = NI_NOFQDN;
	if (nflag) {
		niflag |= NI_NUMERICHOST|NI_NUMERICSERV;
#ifdef NI_WITHSCOPEID
		niflag |= NI_WITHSCOPEID;
#endif
	}
	if (strcmp(proto, "udp") == 0)
		niflag |= NI_DGRAM;
	fillscopeid((struct sockaddr *)&su);
	if (getnameinfo((struct sockaddr *)&su, su.sa.sa_len, h,
	    h ? sizeof(host) : 0, s, s ? sizeof(serv) : 0, niflag)) {
		warnx("inetprint: getnameinfo failed");
		return;
	}
	host[34] = serv[16] = 0;	/*XXX*/

	if (!Pflag) {
		host[(Aflag && !nflag) ? 12 : 16] = 0;
		serv[8] = 0;
	}

	snprintf(line, sizeof(line), "%s.%s", h ? h : "*", s ? s : "*");

	width = Aflag ? 18 : 22;
	printf(" %-*.*s", PARG(width, width), line);
}

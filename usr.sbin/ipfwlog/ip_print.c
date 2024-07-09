/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ip_print.c,v 1.6 1999/09/02 17:03:19 polk Exp
 */
#define	IPFW
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ipfw.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <time.h>
#include <stdio.h>
#include <unistd.h>

void
ip_print(unsigned char *b, int n)
{
	struct sockaddr_in sin;
	struct ip *ip;
	struct udphdr *udp;
	struct tcphdr *tcp;
	struct icmp *icmp;
	int i, s;
	int code;
	char host[1025];
	char port[32];
	extern int nflag;

	if (n < sizeof(*ip)) {
		for (i = 0; i < n; ++i)
			log(" %02x", b[i]);
		n = 0;
		return;
	}
	ip = (struct ip *)b;
	n -= sizeof(*ip);
	b += sizeof(*ip);
	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(struct sockaddr_in);

	if (nflag) {
		log(" %15s -> ", inet_ntoa(ip->ip_src));
		log("%15s ", inet_ntoa(ip->ip_dst));
	} else {
		sin.sin_addr = ip->ip_src;
		if (getnameinfo(&sin, sizeof(sin), host, sizeof(host), NULL, 0, 0) == 0)
			log(" %15s -> ", host);
		else
			log(" %15s -> ", inet_ntoa(ip->ip_src));

		sin.sin_addr = ip->ip_dst;
		if (getnameinfo(&sin, sizeof(sin), host, sizeof(host), NULL, 0, 0) == 0)
			log("%15s ", host);
		else
			log("%15s ", inet_ntoa(ip->ip_src));
	}

	log("%c", ntohs(ip->ip_off) & IP_RF ? 'R':' ');
	log("%c", ntohs(ip->ip_off) & IP_DF ? 'D':' ');
	log("%c ", ntohs(ip->ip_off) & IP_MF ? 'M':' ');
	if ((ntohs(ip->ip_off) & IP_OFFMASK) != 0) {
		log("frag @ %d", (ntohs(ip->ip_off) & IP_OFFMASK) << 3);
		return;
	}

	switch (ip->ip_p) {
	case IPPROTO_UDP:
		log("UDP ");
		if (n < sizeof(struct udphdr)) {
			for (i = 0; i < n; ++i)
				log(" %02x", b[i]);
		} else {
			udp = (struct udphdr *)b;
			log("%5d -> %5d", ntohs(udp->uh_sport),
			    ntohs(udp->uh_dport));
		}
		break;
	case IPPROTO_TCP:
		log("TCP ");
		if (n < sizeof(struct tcphdr)) {
			for (i = 0; i < n; ++i)
				log(" %02x", b[i]);
		} else {
			tcp = (struct tcphdr *)b;
			if (nflag) {
				log("%5d -> %5d", ntohs(tcp->th_sport),
				    ntohs(tcp->th_dport));
			} else {
				sin.sin_port = tcp->th_sport;
				if (getnameinfo(&sin, sizeof(sin), NULL, 0, port, sizeof(port), 0) == 0)
					log(" %5s -> ", port);
				else
					log(" %5d -> ", ntohs(tcp->th_sport));
				sin.sin_port = tcp->th_dport;
				if (getnameinfo(&sin, sizeof(sin), NULL, 0, port, sizeof(port), 0) == 0)
					log("%5s ", port);
				else
					log("%5d ", ntohs(tcp->th_dport));
			}
			log("%c", tcp->th_flags & TH_FIN ? 'F':' ');
			log("%c", tcp->th_flags & TH_SYN ? 'S':' ');
			log("%c", tcp->th_flags & TH_RST ? 'R':' ');
			log("%c", tcp->th_flags & TH_PUSH ? 'P':' ');
			log("%c", tcp->th_flags & TH_ACK ? 'A':' ');
			log("%c", tcp->th_flags & TH_URG ? 'U':' ');
			log("%c", tcp->th_flags & 0x40 ? '4':' ');
			log("%c", tcp->th_flags & 0x80 ? '8':' ');
		}
		break;
	case IPPROTO_ICMP:
		log("ICMP");
		if (n < 4) {
			for (i = 0; i < n; ++i)
				log(" %02x", b[i]);
		} else {
			icmp = (struct icmp *)b;
			switch (icmp->icmp_type) {
			case ICMP_ECHOREPLY:
				log(" ECHO REPLY");
				break;
			case ICMP_UNREACH:
				log(" UNREACH");
				switch (icmp->icmp_code) {
				case ICMP_UNREACH_NET:
					log(" NET");
					break;
				case ICMP_UNREACH_HOST:
					log(" HOST");
					break;
				case ICMP_UNREACH_PROTOCOL:
					log(" PROTOCOL");
					break;
				case ICMP_UNREACH_PORT:
					log(" PORT");
					break;
				case ICMP_UNREACH_NEEDFRAG:
					log(" NEEDFRAG");
					break;
				case ICMP_UNREACH_SRCFAIL:
					log(" SRCFAIL");
					break;
				case ICMP_UNREACH_NET_UNKNOWN:
					log(" NET_UNKNOWN");
					break;
				case ICMP_UNREACH_HOST_UNKNOWN:
					log(" HOST_UNKNOWN");
					break;
				case ICMP_UNREACH_ISOLATED:
					log(" ISOLATED");
					break;
				case ICMP_UNREACH_NET_PROHIB:
					log(" NET_PROHIB");
					break;
				case ICMP_UNREACH_HOST_PROHIB:
					log(" HOST_PROHIB");
					break;
				case ICMP_UNREACH_TOSNET:
					log(" TOSNET");
					break;
				case ICMP_UNREACH_TOSHOST:
					log(" TOSHOST");
					break;
				default:
					log(" 0x%02x",
					    icmp->icmp_code);
					break;
				}
				break;
			case ICMP_SOURCEQUENCH:
				log(" QUENCH");
				break;
			case ICMP_REDIRECT:
				log(" REDIRECT");
				switch (icmp->icmp_code) {
				case ICMP_REDIRECT_NET:
					log(" NET");
					break;
				case ICMP_REDIRECT_HOST:
					log(" HOST");
					break;
				case ICMP_REDIRECT_TOSNET:
					log(" TOSNET");
					break;
				case ICMP_REDIRECT_TOSHOST:
					log(" TOSHOST");
					break;
				default:
					log(" 0x%02x",
					icmp->icmp_code);
					break;
				}
				break;
			case ICMP_ECHO:
				log(" ECHO");
				break;
			case ICMP_ROUTERADVERT:
				log(" ROUTERADVERT");
				break;
			case ICMP_ROUTERSOLICIT:
				log(" ROUTERSOLICIT");
				break;

			case ICMP_TIMXCEED:
				log(" TIMXCEED");
				switch (icmp->icmp_code) {
				case ICMP_TIMXCEED_INTRANS:
					log(" INTRANS");
					break;
				case ICMP_TIMXCEED_REASS:
					log(" REASS");
					break;
				default:
					log(" 0x%02x",
					icmp->icmp_code);
					break;
				}
				break;
			case ICMP_PARAMPROB:
				log(" PARAMPROB");
				switch (icmp->icmp_code) {
				case ICMP_PARAMPROB_OPTABSENT:
					log(" OPTABSENT");
					break;
				default:
					log(" 0x%02x",
					icmp->icmp_code);
					break;
				}
				break;
			case ICMP_TSTAMP:
				log(" TSTAMP");
				break;
			case ICMP_TSTAMPREPLY:
				log(" TSTAMPREPLY");
				break;
			case ICMP_IREQ:
				log(" IREQ");
				break;
			case ICMP_IREQREPLY:
				log(" IREQREPLY");
				break;
			case ICMP_MASKREQ:
				log(" MASREQ");
				break;
			case ICMP_MASKREPLY:
				log(" MASKREPLY");
				break;
			default:
				log(" 0x%02x/0x%02x",
				    icmp->icmp_type, icmp->icmp_code);
			}
			log(" (%d)", icmp->icmp_seq);
		}
		break;
	default:
		log("P%d", ip->ip_p);
	}
}

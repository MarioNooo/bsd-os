/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ip6_print.c,v 1.4 2000/05/03 16:07:47 dab Exp
 */
#define	IPFW
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/ipfw.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <time.h>
#include <stdio.h>
#include <unistd.h>

void
ip6_print(unsigned char *b, int n)
{
	struct ip6_hdr *ip;
	struct udphdr *udp;
	struct tcphdr *tcp;
	struct icmp6_hdr *icmp;
	int i, s;
	int code;
	char N[8*5+1];

	if (n < sizeof(*ip)) {
		for (i = 0; i < n; ++i)
			log(" %02x", b[i]);
		n = 0;
		return;
	}
	ip = (struct ip6_hdr *)b;
	n -= sizeof(*ip);
	b += sizeof(*ip);

	log(" %15s -> ", inet_ntop(AF_INET6, &ip->ip6_src, N, sizeof(N)));
	log("%15s ", inet_ntop(AF_INET6, &ip->ip6_dst, N, sizeof(N)));

	switch (ip->ip6_nxt) {
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
			log("%5d -> %5d", ntohs(tcp->th_sport),
			    ntohs(tcp->th_dport));
			log("%c", tcp->th_flags & TH_FIN ? 'F':' ');
			log("%c", tcp->th_flags & TH_SYN ? 'S':' ');
			log("%c", tcp->th_flags & TH_RST ? 'R':' ');
			log("%c", tcp->th_flags & TH_PUSH ? 'P':' ');
			log("%c", tcp->th_flags & TH_ACK ? 'A':' ');
			log("%c", tcp->th_flags & TH_URG ? 'U':' ');
		}
		break;
	case IPPROTO_ICMPV6:
		log("ICMP6");
		if (n < 4) {
			for (i = 0; i < n; ++i)
				log(" %02x", b[i]);
		} else {
			icmp = (struct icmp6_hdr *)b;
			switch (icmp->icmp6_type) {
			case ICMP6_DST_UNREACH:
				log(" UNREACH");
				switch (icmp->icmp6_code) {
				case ICMP6_DST_UNREACH_NOROUTE:
					log(" NOROUTE");
					break;
				case ICMP6_DST_UNREACH_ADMIN:
					log(" ADMIN");
					break;
				case ICMP6_DST_UNREACH_NOTNEIGHBOR:
					log(" NOTNEIGHBOR");
					break;
				case ICMP6_DST_UNREACH_ADDR:
					log(" ADDRESS");
					break;
				case ICMP6_DST_UNREACH_NOPORT:
					log(" NOPORT");
					break;
				default:
					log(" 0x%02x",
					    icmp->icmp6_code);
					break;
				}
				break;
			case ICMP6_PACKET_TOO_BIG:
				log(" TOOBIG");
				break;

			case ICMP6_TIME_EXCEEDED:
				log(" TIME EXCEEDED");
				switch (icmp->icmp6_code) {
				case ICMP6_TIME_EXCEED_TRANSIT:
					log(" TRANSIT");
					break;
				case ICMP6_TIME_EXCEED_REASSEMBLY:
					log(" REASS");
					break;
				default:
					log(" 0x%02x",
					icmp->icmp6_code);
					break;
				}
				break;
			case ICMP6_PARAM_PROB:
				log(" PARAMPROB");
				switch (icmp->icmp6_code) {
				case ICMP6_PARAMPROB_HEADER:
					log(" HEADER");
					break;
				case ICMP6_PARAMPROB_NEXTHEADER:
					log(" NEXTHDR");
					break;
				case ICMP6_PARAMPROB_OPTION:
					log(" OPTION");
					break;
				default:
					log(" 0x%02x",
					icmp->icmp6_code);
					break;
				}
				break;

			case ICMP6_ECHO_REQUEST:
				log(" ECHO");
				break;
			case ICMP6_ECHO_REPLY:
				log(" ECHO REPLY");
				break;
			case ICMP6_MEMBERSHIP_QUERY:
				log(" GRP QUERY");
				break;
			case ICMP6_MEMBERSHIP_REPORT:
				log(" GRP REPORT");
				break;
			case ICMP6_MEMBERSHIP_REDUCTION:
				log(" GRP TERM");
				break;
			case ND_ROUTER_ADVERT:
				log(" ROUTER ADV");
				break;
			case ND_ROUTER_SOLICIT:
				log(" ROUTER SOL");
				break;
			case ND_NEIGHBOR_ADVERT:
				log(" NEIGHBOR ADV");
				break;
			case ND_NEIGHBOR_SOLICIT:
				log(" NEIGHBOR SOL");
				break;
			case ND_REDIRECT:
				log(" REDIRECT");
				break;
			default:
				log(" 0x%02x/0x%02x",
				    icmp->icmp6_type, icmp->icmp6_code);
				break;
			}
		}
		break;
	default:
		log("P0x%02x", ip->ip6_nxt);
	}
}

/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI lex.l,v 1.17 2003/10/09 18:23:04 prb Exp
 */
%{
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/bpf.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <netinet/tcp.h>
#include <ctype.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include "types.h"
#include "y.tab.h"

int error = 0;
int linecnt = 0;
char *srcfile;
int state;
%}

%s port
%s addr
%s tcpflag
%s icmptype
%s icmpcode
%s paramprob
%s redirect
%s timxceed
%s unreach
%s user
%s interface
%s time


%%
	switch (state) {
	case DSTPORT: case SRCPORT:
		BEGIN(port);
		break;
	case NEXTHOP: case DSTADDR: case SRCADDR:
		BEGIN(addr);
		break;
	case TCPFLAGS:
		BEGIN(tcpflag);
		break;
	case ICMPTYPE: case ICMP:
		BEGIN(icmptype);
		break;
	case ICMPCODE:
		BEGIN(icmpcode);
		break;
	case ICMP_PARAMPROB:
		BEGIN(paramprob);
		break;
	case ICMP_REDIRECT:
		BEGIN(redirect);
		break;
	case ICMP_TIMXCEED:
		BEGIN(timxceed);
		break;
	case ICMP_UNREACH:
		BEGIN(unreach);
		break;
	case USER:
		BEGIN(user);
		break;
	case INTERFACE:
		BEGIN(interface);
		break;
	case TIME:
		BEGIN(time);
		break;
	default:
		BEGIN(INITIAL);
		break;
	}

0[Xx][[:xdigit:]]+	{ yylval.k = strtoul(yytext, 0, 16); return NUMBER; }
0[0-7]*			{ yylval.k = strtoul(yytext, 0, 8); return NUMBER; }
[0-9]+			{ yylval.k = strtoul(yytext, 0, 10); return NUMBER; }
Q\#[ \t]*[0-9A-Fa-f:]+  { char *p = yytext + 2;
			  while (*p == ' ' || *p == '\t') 
				++p;
			  if (inet_pton(AF_INET6, p, &yylval.q) == 0) {
				extern char *srcfile;
				yylval.k = 0;
				fprintf(stderr,
				    "%s: %d: %s: invalid IPv6 address\n",
				    srcfile, linecnt, p);
				error = 1;
			  }
			  return QNUMBER;
			}

<time>[0-9]{1,2}:[0-9]{1,2}:[0-9]{1,2} {
			  char *p = yytext;
			  int h = 0, m = 0, s = 0;
		          while (*p != ':')
				h = h * 10 + *p++ - '0';
		          while (*++p != ':')
				m = m * 10 + *p - '0';
		          while (*++p != '\0')
				s = s * 10 + *p - '0';
			  yylval.k = h * 60 *60 + m * 60 + s;
			  return NUMBER;
			}

<time>[0-9]{1,2}:[0-9]{1,2} {
			  char *p = yytext;
			  int h = 0, m = 0;
		          while (*p != ':')
				h = h * 10 + *p++ - '0';
		          while (*++p != '\0')
				m = m * 10 + *p - '0';
			  yylval.k = h * 60 *60 + m * 60;
			  return NUMBER;
			}

<port>[[:alpha:]][-_[:alnum:]]*\/[[:alpha:]][-_[:alnum:]]* {
			struct servent *s;
			char *proto = strchr(yytext, '/');
			*proto++ = '\0';
			if ((s = getservbyname(yytext, proto)) != NULL) {
				yylval.k = htons(s->s_port);
			} else {
				yylval.k = 0;
				fprintf(stderr,
				    "%s: %d: %s/%s: no such service\n",
				    srcfile, linecnt, yytext, proto);
				error = 1;
			}
			return NUMBER;
}

(service|SERVICE)\([-_[:alnum:]]+\/[-_[:alnum:]]+\) {
			struct servent *s;
			char *name = strchr(yytext, '(') + 1;
			char *proto = strchr(name, '/');
			*proto++ = '\0';
			*strchr(proto, ')') = '\0';
			if ((s = getservbyname(name, proto)) != NULL) {
				yylval.k = htons(s->s_port);
			} else {
				yylval.k = 0;
				fprintf(stderr,
				    "%s: %d: %s/%s: no such service\n",
				    srcfile, linecnt, name, proto);
				error = 1;
			}
			return NUMBER;
		}

<addr>[[:alpha:]][-_\.[:alnum:]]+ {
			struct hostent *h;
			if (error) return NUMBER;
			if ((h = gethostbyname(yytext)) != NULL &&
			    h->h_addrtype == AF_INET && h->h_length == 4) {
				yylval.k =
				    (((u_long)h->h_addr[0] & 0xff) << 24) |
				    (((u_long)h->h_addr[1] & 0xff) << 16) |
				    (((u_long)h->h_addr[2] & 0xff) << 8) |
				    (((u_long)h->h_addr[3] & 0xff) << 0);
			} else {
				yylval.k = 0;
				fprintf(stderr,
				    "%s: %d: %s: no such hostname\n",
				    srcfile, linecnt, yytext);
				error = 1;
			}
			return NUMBER;
}

(host|HOST)\([-_\.[:alnum:]]+\) {
			struct hostent *h;
			char *name = strchr(yytext, '(') + 1;
			*strchr(name, ')') = '\0';
			if (error) return NUMBER;
			if ((h = gethostbyname(name)) != NULL &&
			    h->h_addrtype == AF_INET && h->h_length == 4) {
				yylval.k =
				    (((u_long)h->h_addr[0] & 0xff) << 24) |
				    (((u_long)h->h_addr[1] & 0xff) << 16) |
				    (((u_long)h->h_addr[2] & 0xff) << 8) |
				    (((u_long)h->h_addr[3] & 0xff) << 0);
			} else {
				yylval.k = 0;
				fprintf(stderr,
				    "%s: %d: %s: no such hostname\n",
				    srcfile, linecnt, name);
				error = 1;
			}
			return NUMBER;
		}

<tcpflag>fin	|
<tcpflag>FIN	{ yylval.k = TH_FIN; return NUMBER; }
<tcpflag>syn	|
<tcpflag>SYN	{ yylval.k = TH_SYN; return NUMBER; }
<tcpflag>rst	|
<tcpflag>RST	{ yylval.k = TH_RST; return NUMBER; }
<tcpflag>push	|
<tcpflag>PUSH	{ yylval.k = TH_PUSH; return NUMBER; }
<tcpflag>ack	|
<tcpflag>ACK	{ yylval.k = TH_ACK; return NUMBER; }
<tcpflag>urg	|
<tcpflag>URG	{ yylval.k = TH_URG; return NUMBER; }

ip		|
IP		{ yylval.k = IPPROTO_IP; return IP; }
icmp		|
ICMP		{ yylval.k = IPPROTO_ICMP; return ICMP; }
igmp		|
IGMP		{ yylval.k = IPPROTO_IGMP; return IGMP; }
ggp		|
GGP		{ yylval.k = IPPROTO_GGP; return GGP; }
ipip		|
IPIP		{ yylval.k = IPPROTO_IPIP; return IPIP; }
tcp		|
TCP		{ yylval.k = IPPROTO_TCP; return TCP; }
egp		|
EGP		{ yylval.k = IPPROTO_EGP; return EGP; }
pup		|
PUP		{ yylval.k = IPPROTO_PUP; return PUP; }
udp		|
UDP		{ yylval.k = IPPROTO_UDP; return UDP; }
idp		|
IDP		{ yylval.k = IPPROTO_IDP; return IDP; }
tp		|
TP		{ yylval.k = IPPROTO_TP; return TP; }
eon		|
EON		{ yylval.k = IPPROTO_EON; return EON; }
encap		|
ENCAP		{ yylval.k = IPPROTO_ENCAP; return ENCAP; }
ospf		|
OSPF		{ yylval.k = 89; return OSPF; }
IPV6ROUTE	|
IPv6ROUTE	|
ipv6route	|
ROUTING		|
routing		{ yylval.k = IPPROTO_ROUTING; return ROUTING; }
IPV6FRAG	|
IPv6FRAG	|
ipv6frag	|
FRAGMENT	|
fragment	{ yylval.k = IPPROTO_FRAGMENT; return FRAGMENT; }
IPV6NONXT	|
IPv6NONXT	|
ipv6nonxt	|
NONE		|
none		{ yylval.k = IPPROTO_NONE; return NONE; }
IPV6OPTS	|
IPv6OPTS	|
ipv6opts	|
DSTOPTS		|
dstopts		{ yylval.k = IPPROTO_DSTOPTS; return DSTOPTS; }
esp		|
ESP		{ yylval.k = IPPROTO_ESP; return ESP; }
ah		|
AH		{ yylval.k = IPPROTO_AH; return AH; }
icmpv6		|
ICMPv6		|
ICMPV6		{ yylval.k = IPPROTO_ICMPV6; return ICMPV6; }

<icmptype>echo		|
<icmptype>ECHO		{ yylval.k = ICMP_ECHO; return NUMBER; }

<icmptype>echoreply	|
<icmptype>ECHOREPLY	{ yylval.k = ICMP_ECHOREPLY; return NUMBER; }

<icmptype>ireq		|
<icmptype>IREQ		{ yylval.k = ICMP_IREQ; return NUMBER; }

<icmptype>ireqreply	|
<icmptype>IREQREPLY	{ yylval.k = ICMP_IREQREPLY; return NUMBER; }

<icmptype>maskreply	|
<icmptype>MASKREPLY	{ yylval.k = ICMP_MASKREPLY; return NUMBER; }

<icmptype>maskreq	|
<icmptype>MASKREQ	{ yylval.k = ICMP_MASKREQ; return NUMBER; }

<icmptype>paramprob	|
<icmptype>PARAMPROB	{ yylval.k = ICMP_PARAMPROB; return NUMBER; }

<icmpcode>paramprob_optabsent |
<icmpcode>PARAMPROB_OPTABSENT |
<paramprob>optabsent	|
<paramprob>OPTABSENT	{ yylval.k = ICMP_PARAMPROB_OPTABSENT; return NUMBER; }

<icmptype>redirect	|
<icmptype>REDIRECT	{ yylval.k = ICMP_REDIRECT; return NUMBER; }

<icmpcode>redirect_host	|
<icmpcode>REDIRECT_HOST	|
<redirect>host		|
<redirect>HOST		{ yylval.k = ICMP_REDIRECT_HOST; return NUMBER; }

<icmpcode>redirect_net	|
<icmpcode>REDIRECT_NET	|
<redirect>net		|
<redirect>NET		{ yylval.k = ICMP_REDIRECT_NET; return NUMBER; }

<icmpcode>redirect_toshost |
<icmpcode>REDIRECT_TOSHOST |
<redirect>toshost 	|
<redirect>TOSHOST	{ yylval.k = ICMP_REDIRECT_TOSHOST; return NUMBER; }

<icmpcode>redirect_tosnet	|
<icmpcode>REDIRECT_TOSNET	|
<redirect>tosnet	|
<redirect>TOSNET	{ yylval.k = ICMP_REDIRECT_TOSNET; return NUMBER; }

<icmptype>routeradvert	|
<icmptype>ROUTERADVERT	{ yylval.k = ICMP_ROUTERADVERT; return NUMBER; }

<icmptype>routersolicit	|
<icmptype>ROUTERSOLICIT	{ yylval.k = ICMP_ROUTERSOLICIT; return NUMBER; }

<icmptype>sourcequench	|
<icmptype>SOURCEQUENCH	{ yylval.k = ICMP_SOURCEQUENCH; return NUMBER; }

<icmptype>timxceed	|
<icmptype>TIMXCEED	{ yylval.k = ICMP_TIMXCEED; return NUMBER; }

<icmpcode>timxceed_intrans |
<icmpcode>TIMXCEED_INTRANS |
<timxceed>intrans 	|
<timxceed>INTRANS 	{ yylval.k = ICMP_TIMXCEED_INTRANS; return NUMBER; }

<icmpcode>timxceed_reass	|
<icmpcode>TIMXCEED_REASS	|
<timxceed>reass		|
<timxceed>REASS		{ yylval.k = ICMP_TIMXCEED_REASS; return NUMBER; }

<icmptype>tstamp	|
<icmptype>TSTAMP	{ yylval.k = ICMP_TSTAMP; return NUMBER; }

<icmptype>tstampreply	|
<icmptype>TSTAMPREPLY	{ yylval.k = ICMP_TSTAMPREPLY; return NUMBER; }

<icmptype>unreach	|
<icmptype>UNREACH	{ yylval.k = ICMP_UNREACH; return NUMBER; }

<icmpcode>unreach_host	|
<icmpcode>UNREACH_HOST	|
<unreach>host		|
<unreach>HOST		{ yylval.k = ICMP_UNREACH_HOST; return NUMBER; }

<icmpcode>unreach_host_prohib |
<icmpcode>UNREACH_HOST_PROHIB |
<unreach>host_prohib |
<unreach>HOST_PROHIB { yylval.k = ICMP_UNREACH_HOST_PROHIB; return NUMBER; }

<icmpcode>unreach_host_unknown |
<icmpcode>UNREACH_HOST_UNKNOWN |
<unreach>host_unknown |
<unreach>HOST_UNKNOWN { yylval.k = ICMP_UNREACH_HOST_UNKNOWN; return NUMBER; }

<icmpcode>unreach_isolated |
<icmpcode>UNREACH_ISOLATED |
<unreach>isolated |
<unreach>ISOLATED { yylval.k = ICMP_UNREACH_ISOLATED; return NUMBER; }

<icmpcode>unreach_needfrag |
<icmpcode>UNREACH_NEEDFRAG |
<unreach>needfrag |
<unreach>NEEDFRAG { yylval.k = ICMP_UNREACH_NEEDFRAG; return NUMBER; }

<icmpcode>unreach_net	|
<icmpcode>UNREACH_NET	|
<unreach>net	|
<unreach>NET	{ yylval.k = ICMP_UNREACH_NET; return NUMBER; }

<icmpcode>unreach_net_prohib |
<icmpcode>UNREACH_NET_PROHIB |
<unreach>net_prohib |
<unreach>NET_PROHIB { yylval.k = ICMP_UNREACH_NET_PROHIB; return NUMBER; }

<icmpcode>unreach_net_unknown		|
<icmpcode>UNREACH_NET_UNKNOWN |
<unreach>net_unknown		|
<unreach>NET_UNKNOWN { yylval.k = ICMP_UNREACH_NET_UNKNOWN; return NUMBER; }

<icmpcode>unreach_port	|
<icmpcode>UNREACH_PORT	|
<unreach>port	|
<unreach>PORT	{ yylval.k = ICMP_UNREACH_PORT; return NUMBER; }

<icmpcode>unreach_protocol |
<icmpcode>UNREACH_PROTOCOL |
<unreach>protocol |
<unreach>PROTOCOL { yylval.k = ICMP_UNREACH_PROTOCOL; return NUMBER; }

<icmpcode>unreach_srcfail	|
<icmpcode>UNREACH_SRCFAIL	|
<unreach>srcfail	|
<unreach>SRCFAIL	{ yylval.k = ICMP_UNREACH_SRCFAIL; return NUMBER; }

<icmpcode>unreach_toshost	|
<icmpcode>UNREACH_TOSHOST	|
<unreach>toshost	|
<unreach>TOSHOST	{ yylval.k = ICMP_UNREACH_TOSHOST; return NUMBER; }

<icmpcode>unreach_tosnet	|
<icmpcode>UNREACH_TOSNET	|
<unreach>tosnet	|
<unreach>TOSNET	{ yylval.k = ICMP_UNREACH_TOSNET; return NUMBER; }

<icmptype>dst_unreach	|
<icmptype>DST_UNREACH	{ yylval.k = ICMP6_DST_UNREACH; return NUMBER; }
<icmptype>packet_too_big |
<icmptype>PACKET_TOO_BIG { yylval.k = ICMP6_PACKET_TOO_BIG; return NUMBER; }
<icmptype>time_exceeded	|
<icmptype>TIME_EXCEEDED	{ yylval.k = ICMP6_TIME_EXCEEDED; return NUMBER; }
<icmptype>packet_to_big	|
<icmptype>PARAM_PROB	{ yylval.k = ICMP6_PARAM_PROB; return NUMBER; }
<icmptype>echo_request	|
<icmptype>ECHO_REQUEST	{ yylval.k = ICMP6_ECHO_REQUEST; return NUMBER; }
<icmptype>echo_reply	|
<icmptype>ECHO_REPLY	{ yylval.k = ICMP6_ECHO_REPLY; return NUMBER; }
<icmptype>membership_query |
<icmptype>MEMBERSHIP_QUERY { yylval.k = ICMP6_MEMBERSHIP_QUERY; return NUMBER; }
<icmptype>membership_report |
<icmptype>MEMBERSHIP_REPORT { yylval.k = ICMP6_MEMBERSHIP_REPORT; return NUMBER; }
<icmptype>membership_reduction |
<icmptype>MEMBERSHIP_REDUCTION { yylval.k = ICMP6_MEMBERSHIP_REDUCTION; return NUMBER; }
<icmptype>router_solicit |
<icmptype>ROUTER_SOLICIT { yylval.k = ND_ROUTER_SOLICIT; return NUMBER; }
<icmptype>router_advert |
<icmptype>ROUTER_ADVERT { yylval.k = ND_ROUTER_ADVERT; return NUMBER; }
<icmptype>neighbor_solicit |
<icmptype>NEIGHBOR_SOLICIT { yylval.k = ND_NEIGHBOR_SOLICIT; return NUMBER; }
<icmptype>neighbor_advert |
<icmptype>NEIGHBOR_ADVERT { yylval.k = ND_NEIGHBOR_ADVERT; return NUMBER; }
<icmptype>redirect6 |
<icmptype>REDIRECT6 { yylval.k = ND_REDIRECT; return NUMBER; }

<icmptype>router_renumbering |
<icmptype>ROUTER_RENUMBERING { yylval.k = ICMP6_ROUTER_RENUMBERING; return NUMBER; }
<icmptype>wrurequest |
<icmptype>WRUREQUEST { yylval.k = ICMP6_WRUREQUEST; return NUMBER; }
<icmptype>wrureply |
<icmptype>WRUREPLY { yylval.k = ICMP6_WRUREPLY; return NUMBER; }
<icmptype>fqdn_query |
<icmptype>FQDN_QUERY { yylval.k = ICMP6_FQDN_QUERY; return NUMBER; }
<icmptype>fqdn_reply |
<icmptype>FQDN_REPLY { yylval.k = ICMP6_FQDN_REPLY; return NUMBER; }
<icmptype>ni_query |
<icmptype>NI_QUERY { yylval.k = ICMP6_NI_QUERY; return NUMBER; }
<icmptype>ni_reply |
<icmptype>NI_REPLY { yylval.k = ICMP6_NI_REPLY; return NUMBER; }


(add|ADD|and|AND|div|DIV|jeq|JEQ|jge|JGE|jgt|JGT|jle|JLE|jlt|JLT|jmp|JMP|jne|JNE|jset|JSET|jnet|JNET|ldb|LDB|ldh|LDH|ldx|LDX|ld|LD|len|LEN|lsh|LSH|mul|MUL|neg|NEG|or|OR|ret|RET|rsh|RSH|stx|STX|st|ST|sub|SUB|tax|TAX|txa|TXA)[ \t]+[^}\n]* {
			yylval.cmds = malloc(sizeof(cmd_t));
			yylval.cmds->cmd = malloc(strlen(yytext)+2);
			yylval.cmds->cmd[0] = '\t';
			yylval.cmds->line = linecnt;
			yylval.cmds->file = srcfile;
			strcpy(yylval.cmds->cmd+1, yytext);
			yylval.cmds->next = NULL;
			return CMD;
		}
^[lL][[:digit:]]+:	{
			yylval.cmds = malloc(sizeof(cmd_t));
			yylval.cmds->cmd = strdup(yytext);
			yylval.cmds->next = NULL;
			yylval.cmds->line = linecnt;
			yylval.cmds->file = srcfile;
			return CMD;
		}

<user>userbit[0-7]	|
<user>USERBIT[0-7]	{
			yylval.k = 0x00010000 << (yytext[7] - '0');
			return USER;
		}

<user>user[[:digit:]]+	|
<user>user0[xX][[:xdigit:]]+ |
<user>USER[[:digit:]]+ |
<user>USER0[xX][[:xdigit:]]+ {
			yylval.k = strtol(yytext + 4, 0, 0);
			if (yylval.k > 255) {
				fprintf(stderr,
				    "%s: %d: %s: value out of range\n",
				    srcfile, linecnt, yytext);
				error = 1;
			}
			yylval.k <<= 16;
			return USER;
		}

else		|
ELSE		{ yylval.k = linecnt; return ELSE; }
case		|
CASE		{ yylval.k = linecnt; return CASE; }
switch		|
SWITCH		{ yylval.k = linecnt; return SWITCH; }
default		|
DEFAULT		{ yylval.k = linecnt; return DEFAULT; }
block		|
BLOCK		{ yylval.k = linecnt; return BLOCK; }
break		|
BREAK		{ yylval.k = linecnt; return BREAK; }

accumulator	|
ACCUMULATOR	{ yylval.k = linecnt; return ACCUMULATOR; }
preheader	|
PREHEADER	{ yylval.k = linecnt; return PREHEADER; }
decapsulated	|
DECAPSULATED	{ yylval.k = linecnt; return DECAPSULATED; }
toobig		|
TOOBIG		{ yylval.k = linecnt; return TOOBIG; }
nexthop		|
NEXTHOP		{ yylval.k = linecnt; return NEXTHOP; }
dstaddr		|
DSTADDR		{ yylval.k = linecnt; return DSTADDR; }
dstport		|
DSTPORT		{ yylval.k = linecnt; return DSTPORT; }
interface	|
INTERFACE	{ yylval.k = linecnt; return INTERFACE; }
ipprotocol	|
IPPROTOCOL	{ yylval.k = linecnt; return PROTOCOL; }
srcaddr		|
SRCADDR		{ yylval.k = linecnt; return SRCADDR; }
srcport		|
SRCPORT		{ yylval.k = linecnt; return SRCPORT; }
tcpflags	|
TCPFLAGS	{ yylval.k = linecnt; return TCPFLAGS; }
icmptype	|
ICMPTYPE	{ yylval.k = linecnt; return ICMPTYPE; }
icmpcode	|
ICMPCODE	{ yylval.k = linecnt; return ICMPCODE; }
ipdata		|
IPDATA		{ yylval.k = linecnt; return IPDATA; }
data		|
DATA		{ yylval.k = linecnt; return DATA; }
ipversion	|
IPVERSION	{ yylval.k = linecnt; return IPV; }
ipv4		|
IPv4		|
IPV4		{ yylval.k = linecnt; return IPV4; }
ipv6		|
IPv6		|
IPV6		{ yylval.k = linecnt; return IPV6; }
ipv4inipv4	|
IPV4inIPV4	|
IPv4inIPv4	|
IPV4INIPV4	{ yylval.k = linecnt; return IPV4inIPV4; }
IPV4inIPV6	|
IPv4inIPv6	|
IPV4INIPV6	{ yylval.k = linecnt; return IPV4inIPV6; }
IPV6inIPV4	|
IPv6inIPv4	|
IPV6INIPV4	{ yylval.k = linecnt; return IPV6inIPV4; }
IPV6inIPV6	|
IPv6inIPv6	|
IPV6INIPV6	{ yylval.k = linecnt; return IPV6inIPV6; }
ipoffset	|
IPOFFSET	{ yylval.k = linecnt; return IPOFFSET; }
ipfrag		|
IPFRAG		{ yylval.k = linecnt; return IPFRAG; }
ipdontfrag	|
IPDONTFRAG	{ yylval.k = linecnt; return IPDONTFRAG; }
ipfirstfrag	|
IPFIRSTFRAG	{ yylval.k = linecnt; return IPFIRSTFRAG; }
ipmorefrag	|
IPMOREFRAG	{ yylval.k = linecnt; return IPMOREFRAG; }
iphlen		|
IPHLEN		{ yylval.k = linecnt; return IPHLEN; }
ipdatalen	|
IPDATALEN	{ yylval.k = linecnt; return IPDATALEN; }
iplen		|
IPLEN		{ yylval.k = linecnt; return IPLEN; }
established	|
ESTABLISHED	{ yylval.k = linecnt; return ESTABLISHED; }
request		|
REQUEST		{ yylval.k = linecnt; return REQUEST; }
forwarding	|
FORWARDING	{ yylval.k = linecnt; return FORWARDING; }
broadcast	|
BROADCAST	{ yylval.k = linecnt; return BROADCAST; }
bind		|
BIND		{ yylval.k = linecnt; return BIND; }
packetlength	|
PACKETLENGTH	{ yylval.k = linecnt; return PACKETLENGTH; }

chksrc		|
CHKSRC		{ yylval.k = linecnt; return CHKSRC; }
deny		|
DENY		{ yylval.k = linecnt; return DENY; }
reject		|
REJECT		{ yylval.k = linecnt; return DENY; }
accept		|
ACCEPT		{ yylval.k = linecnt; return PERMIT; }
forcelocal	|
FORCELOCAL	{ yylval.k = linecnt; return LOCAL; }
fastforward	|
FASTFORWARD	{ yylval.k = linecnt; return FASTFWD; }
permit		|
PERMIT		{ yylval.k = linecnt; return PERMIT; }
next		|
NEXT		{ yylval.k = linecnt; return NEXT; }
filter		|
FILTER		{ yylval.k = linecnt; return FILTER; }
input		|
INPUT		{ yylval.k = linecnt; return INPUT; }
output		|
OUTPUT		{ yylval.k = linecnt; return OUTPUT; }
return		|
RETURN		{ yylval.k = linecnt; return RETURN; }
set		|
SET		{ yylval.k = linecnt; return SET; }
change		|
CHANGE		{ yylval.k = linecnt; return CHANGE; }
from		|
FROM		{ yylval.k = linecnt; return FROM; }
to		|
TO		{ yylval.k = linecnt; return TO; }
time		|
TIME		{ yylval.k = linecnt; return TIME; }
day		|
DAY		{ yylval.k = linecnt; return DAY; }

call		|
CALL		{ yylval.k = linecnt; return CALL; }
chain		|
CHAIN		{ yylval.k = linecnt; return CHAIN; }

monday		|
Monday		|
MONDAY		|
MON		|
mon		|
Mon		{ yylval.k = 0; return NUMBER; }

tuesday		|
Tuesday		|
TUESDAY		|
TUE		|
tue		|
Tue		{ yylval.k = 1; return NUMBER; }

wednesday	|
Wednesday	|
WEDNESDAY	|
WED		|
wed		|
Wed		{ yylval.k = 2; return NUMBER; }

thursday	|
Thursday	|
THURSDAY	|
THU		|
thu		|
Thu		{ yylval.k = 3; return NUMBER; }

friday		|
fri		|
FRIDAY		|
FRI		|
Friday		|
Fri		{ yylval.k = 4; return NUMBER; }

saturday	|
Saturday	|
SATURDAY	|
SAT		|
sat		|
Sat		{ yylval.k = 5; return NUMBER; }

sunday		|
Sunday		|
SUNDAY		|
SUN		|
sun		|
Sun		{ yylval.k = 6; return NUMBER; }

implicit	|
IMPLICIT	return IMPLICIT;

or		|
OR		|
"||"		return OR;
and		|
AND		|
"&&"		return AND;
\!		|
not		|
NOT		return NOT;

<interface>[[:lower:]]+[[:digit:]]+ {
			yylval.cmds = malloc(sizeof(cmd_t));
			yylval.cmds->cmd = strdup(yytext);
			yylval.cmds->next = NULL;
			return IFACE;
		}

^#[ \t]+[[:digit:]]+[ \t]+\"[^\"\n]*\".*\n {
			char *p = yytext;

			linecnt = atol(p+1);
			p = strchr(p, '"') + 1;
			*strchr(p, '"') = '\0';
			if (*p)
				srcfile = strdup(p);
			else
				srcfile = "<STDIN>";
		}

\/\/.*\n		++linecnt;
^#.*\n			++linecnt;
[ \t]+			;
\n			++linecnt;
[-=<>&{}()\[\],\./:;]		return yytext[0];
\"[_[:alpha:]][-_@/\.[:alnum:]]*\" {
			yylval.s = strdup(yytext+1);
			*strchr(yylval.s, '"') = '\0';
			return STRING;
		}
[_[:alpha:]][-_@/\.[:alnum:]]* {
			yylval.s = strdup(yytext);
			return STRING;
		}
.			{ printf("Matched ``%s''\n", yytext); return ERR; }
%%

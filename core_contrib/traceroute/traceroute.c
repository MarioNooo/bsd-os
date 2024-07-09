/*
%%% portions-copyright-cmetz-97
Portions of this software are Copyright 1997 by Craig Metz, All Rights
Reserved. The Inner Net License Version 2 applies to these portions of
the software.
You should have received a copy of the license with this software. If
you didn't get a copy, you may request one from <license@inner.net>.

*/
#define INET6 1
/*
 * Copyright (c) 1988, 1989, 1991, 1994, 1995, 1996, 1997
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
static const char copyright[] =
    "@(#) Copyright (c) 1988, 1989, 1991, 1994, 1995, 1996, 1997\n\
The Regents of the University of California.  All rights reserved.\n";
static const char rcsid[] =
    "@(#)/master/core_contrib/traceroute/traceroute.c,v 1.9 2003/07/15 01:22:21 polk Exp (LBL)";
#endif

/*
 * traceroute host  - trace the route ip packets follow going to "host".
 *
 * Attempt to trace the route an ip packet would follow to some
 * internet host.  We find out intermediate hops by launching probe
 * packets with a small ttl (time to live) then listening for an
 * icmp "time exceeded" reply from a gateway.  We start our probes
 * with a ttl of one and increase by one until we get an icmp "port
 * unreachable" (which means we got to "host") or hit a max (which
 * defaults to 40 hops & can be changed with the -m flag).  Three
 * probes (change with -q flag) are sent at each ttl setting and a
 * line is printed showing the ttl, address of the gateway and
 * round trip time of each probe.  If the probe answers come from
 * different gateways, the address of each responding system will
 * be printed.  If there is no response within a 5 sec. timeout
 * interval (changed with the -w flag), a "*" is printed for that
 * probe.
 *
 * Probe packets are UDP format.  We don't want the destination
 * host to process them so the destination port is set to an
 * unlikely value (if some clod on the destination is using that
 * value, it can be changed with the -p flag).
 *
 * A sample use might be:
 *
 *     [yak 71]% traceroute nis.nsf.net.
 *     traceroute to nis.nsf.net (35.1.1.48), 40 hops max, 56 byte packet
 *      1  helios.ee.lbl.gov (128.3.112.1)  19 ms  19 ms  0 ms
 *      2  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  39 ms  19 ms
 *      3  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  39 ms  19 ms
 *      4  ccngw-ner-cc.Berkeley.EDU (128.32.136.23)  39 ms  40 ms  39 ms
 *      5  ccn-nerif22.Berkeley.EDU (128.32.168.22)  39 ms  39 ms  39 ms
 *      6  128.32.197.4 (128.32.197.4)  40 ms  59 ms  59 ms
 *      7  131.119.2.5 (131.119.2.5)  59 ms  59 ms  59 ms
 *      8  129.140.70.13 (129.140.70.13)  99 ms  99 ms  80 ms
 *      9  129.140.71.6 (129.140.71.6)  139 ms  239 ms  319 ms
 *     10  129.140.81.7 (129.140.81.7)  220 ms  199 ms  199 ms
 *     11  nic.merit.edu (35.1.1.48)  239 ms  239 ms  239 ms
 *
 * Note that lines 2 & 3 are the same.  This is due to a buggy
 * kernel on the 2nd hop system -- lbl-csam.arpa -- that forwards
 * packets with a zero ttl.
 *
 * A more interesting example is:
 *
 *     [yak 72]% traceroute allspice.lcs.mit.edu.
 *     traceroute to allspice.lcs.mit.edu (18.26.0.115), 40 hops max
 *      1  helios.ee.lbl.gov (128.3.112.1)  0 ms  0 ms  0 ms
 *      2  lilac-dmc.Berkeley.EDU (128.32.216.1)  19 ms  19 ms  19 ms
 *      3  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  19 ms  19 ms
 *      4  ccngw-ner-cc.Berkeley.EDU (128.32.136.23)  19 ms  39 ms  39 ms
 *      5  ccn-nerif22.Berkeley.EDU (128.32.168.22)  20 ms  39 ms  39 ms
 *      6  128.32.197.4 (128.32.197.4)  59 ms  119 ms  39 ms
 *      7  131.119.2.5 (131.119.2.5)  59 ms  59 ms  39 ms
 *      8  129.140.70.13 (129.140.70.13)  80 ms  79 ms  99 ms
 *      9  129.140.71.6 (129.140.71.6)  139 ms  139 ms  159 ms
 *     10  129.140.81.7 (129.140.81.7)  199 ms  180 ms  300 ms
 *     11  129.140.72.17 (129.140.72.17)  300 ms  239 ms  239 ms
 *     12  * * *
 *     13  128.121.54.72 (128.121.54.72)  259 ms  499 ms  279 ms
 *     14  * * *
 *     15  * * *
 *     16  * * *
 *     17  * * *
 *     18  ALLSPICE.LCS.MIT.EDU (18.26.0.115)  339 ms  279 ms  279 ms
 *
 * (I start to see why I'm having so much trouble with mail to
 * MIT.)  Note that the gateways 12, 14, 15, 16 & 17 hops away
 * either don't send ICMP "time exceeded" messages or send them
 * with a ttl too small to reach us.  14 - 17 are running the
 * MIT C Gateway code that doesn't send "time exceeded"s.  God
 * only knows what's going on with 12.
 *
 * The silent gateway 12 in the above may be the result of a bug in
 * the 4.[23]BSD network code (and its derivatives):  4.x (x <= 3)
 * sends an unreachable message using whatever ttl remains in the
 * original datagram.  Since, for gateways, the remaining ttl is
 * zero, the icmp "time exceeded" is guaranteed to not make it back
 * to us.  The behavior of this bug is slightly more interesting
 * when it appears on the destination system:
 *
 *      1  helios.ee.lbl.gov (128.3.112.1)  0 ms  0 ms  0 ms
 *      2  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  19 ms  39 ms
 *      3  lilac-dmc.Berkeley.EDU (128.32.216.1)  19 ms  39 ms  19 ms
 *      4  ccngw-ner-cc.Berkeley.EDU (128.32.136.23)  39 ms  40 ms  19 ms
 *      5  ccn-nerif35.Berkeley.EDU (128.32.168.35)  39 ms  39 ms  39 ms
 *      6  csgw.Berkeley.EDU (128.32.133.254)  39 ms  59 ms  39 ms
 *      7  * * *
 *      8  * * *
 *      9  * * *
 *     10  * * *
 *     11  * * *
 *     12  * * *
 *     13  rip.Berkeley.EDU (128.32.131.22)  59 ms !  39 ms !  39 ms !
 *
 * Notice that there are 12 "gateways" (13 is the final
 * destination) and exactly the last half of them are "missing".
 * What's really happening is that rip (a Sun-3 running Sun OS3.5)
 * is using the ttl from our arriving datagram as the ttl in its
 * icmp reply.  So, the reply will time out on the return path
 * (with no notice sent to anyone since icmp's aren't sent for
 * icmp's) until we probe with a ttl that's at least twice the path
 * length.  I.e., rip is really only 7 hops away.  A reply that
 * returns with a ttl of 1 is a clue this problem exists.
 * Traceroute prints a "!" after the time if the ttl is <= 1.
 * Since vendors ship a lot of obsolete (DEC's Ultrix, Sun 3.x) or
 * non-standard (HPUX) software, expect to see this problem
 * frequently and/or take care picking the target host of your
 * probes.
 *
 * Other possible annotations after the time are !H, !N, !P (got a host,
 * network or protocol unreachable, respectively), !S or !F (source
 * route failed or fragmentation needed -- neither of these should
 * ever occur and the associated gateway is busted if you see one).  If
 * almost all the probes result in some kind of unreachable, traceroute
 * will give up and exit.
 *
 * Notes
 * -----
 * This program must be run by root or be setuid.  (I suggest that
 * you *don't* make it setuid -- casual use could result in a lot
 * of unnecessary traffic on our poor, congested nets.)
 *
 * This program requires a kernel mod that does not appear in any
 * system available from Berkeley:  A raw ip socket using proto
 * IPPROTO_RAW must interpret the data sent as an ip datagram (as
 * opposed to data to be wrapped in a ip datagram).  See the README
 * file that came with the source to this program for a description
 * of the mods I made to /sys/netinet/raw_ip.c.  Your mileage may
 * vary.  But, again, ANY 4.x (x < 4) BSD KERNEL WILL HAVE TO BE
 * MODIFIED TO RUN THIS PROGRAM.
 *
 * The udp port usage may appear bizarre (well, ok, it is bizarre).
 * The problem is that an icmp message only contains 8 bytes of
 * data from the original datagram.  8 bytes is the size of a udp
 * header so, if we want to associate replies with the original
 * datagram, the necessary information must be encoded into the
 * udp header (the ip id could be used but there's no way to
 * interlock with the kernel's assignment of ip id's and, anyway,
 * it would have taken a lot more kernel hacking to allow this
 * code to set the ip id).  So, to allow two or more users to
 * use traceroute simultaneously, we use this task's pid as the
 * source port (the high bit is set to move the port number out
 * of the "likely" range).  To keep track of which probe is being
 * replied to (so times and/or hop counts don't get confused by a
 * reply that was delayed in transit), we increment the destination
 * port number before each probe.
 *
 * Don't use this as a coding example.  I was trying to find a
 * routing problem and this code sort-of popped out after 48 hours
 * without sleep.  I was amazed it ever compiled, much less ran.
 *
 * I stole the idea for this program from Steve Deering.  Since
 * the first release, I've learned that had I attended the right
 * IETF working group meetings, I also could have stolen it from Guy
 * Almes or Matt Mathis.  I don't know (or care) who came up with
 * the idea first.  I envy the originators' perspicacity and I'm
 * glad they didn't keep the idea a secret.
 *
 * Tim Seaver, Ken Adelman and C. Philip Wood provided bug fixes and/or
 * enhancements to the original distribution.
 *
 * I've hacked up a round-trip-route version of this that works by
 * sending a loose-source-routed udp datagram through the destination
 * back to yourself.  Unfortunately, SO many gateways botch source
 * routing, the thing is almost worthless.  Maybe one day...
 *
 *  -- Van Jacobson (van@ee.lbl.gov)
 *     Tue Dec 20 03:50:13 PST 1988
 */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#if INET6
#include <sys/uio.h>

#ifndef __bsdi__
#include <netinet6/in6.h>
#endif
#include <netinet6/ipv6.h>
#include <netinet6/icmpv6.h>

typedef union {
  struct sockaddr sa;
  struct sockaddr_in sin;
  struct sockaddr_in6 sin6;
} su;
#endif /* INET6 */

#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <memory.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gnuc.h"
#ifdef HAVE_OS_PROTO_H
#include "os-proto.h"
#endif

#include "ifaddrlist.h"
/*#include "savestr.h"*/

/* Maximum number of gateways (include room for one noop) */
#define NGATEWAYS ((int)((MAX_IPOPTLEN - IPOPT_MINOFF - 1) / sizeof(u_int32_t)))

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	64
#endif

#define Fprintf (void)fprintf
#define Printf (void)printf

/* Host name and address list */
struct hostinfo {
	char *name;
	int n;
	u_int32_t *addrs;
};

/* Data section of the probe packet */
struct outdata {
	u_char seq;		/* sequence number of this packet */
	u_char ttl;		/* ttl packet left with */
	struct timeval tv;	/* time packet left */
};

u_char	packet[512];		/* last inbound (icmp) packet */

#if INET6
void *outbuf;
#endif /* INET6 */
struct ip *outip;		/* last output (udp) packet */
struct udphdr *outudp;		/* last output (udp) packet */
struct outdata *outdata;	/* last output (udp) packet */

struct icmp *outicmp;		/* last output (icmp) packet */

/* loose source route gateway list (including room for final destination) */
u_int32_t gwlist[NGATEWAYS + 1];

int s;				/* receive (icmp) socket file descriptor */
int sndsock;			/* send (udp/icmp) socket file descriptor */

#if INET6
su to, from, lastaddr;

struct msghdr sendmsghdr, recvmsghdr;
struct iovec sendiovec, recviovec;
#ifndef CMSG_LEN
#define CMSG_LEN(x)	(sizeof(struct cmsghdr) + (x))
#endif
#ifndef CMSG_SPACE
#define CMSG_SPACE(x)	(ALIGN(CMSG_LEN(x)))
#endif
char sendcontrol[CMSG_SPACE(sizeof(int)) +
	CMSG_SPACE(sizeof(struct in6_pktinfo))];
char recvcontrol[CMSG_SPACE(sizeof(int))];

u_int16_t sport;
#else /* INET6 */
struct sockaddr whereto;	/* Who to try to reach */
struct sockaddr_in wherefrom;	/* Who we are */
#endif /* INET6 */
int packlen;			/* total length of packet */
int minpacket;			/* min ip packet size */
int maxpacket = 32 * 1024;	/* max ip packet size */

char *prog;
char *source;
char *hostname;
char *device;

int nprobes = 3;
int max_ttl = 40;
int first_ttl = 1;
u_short ident;
u_short port = 32768 + 666;	/* start udp dest port # for probe packets */

int options;			/* socket options */
int verbose;
int waittime = 5;		/* time to wait for response (in seconds) */
int nflag;			/* print addresses numerically */
int useicmp;			/* use icmp echo instead of udp packets */
#ifdef CANT_HACK_CKSUM
int docksum = 0;		/* don't calculate checksums */
#else
int docksum = 1;		/* calculate checksums */
#endif
int optlen;			/* length of ip options */

extern int optind;
extern int opterr;
extern char *optarg;

/* Forwards */
double	deltaT(struct timeval *, struct timeval *);
void	freehostinfo(struct hostinfo *);
void	getaddr(u_int32_t *, char *);
struct	hostinfo *gethostinfo(char *);
u_short	in_cksum(u_short *, int);
char	*inetname(struct in_addr);
int	main(int, char **);
int	packet_ok(u_char *, int, struct sockaddr_in *, int);
char	*pr_type(u_char);
#if INET6
void	print(u_char *, int, struct sockaddr *);
#else /* INET6 */
void	print(u_char *, int, struct sockaddr_in *);
#endif /* INET6 */
void	send_probe(int, int, struct timeval *);
void	setsin(struct sockaddr_in *, u_int32_t);
int	str2val(const char *, const char *, int, int);
void	tvsub(struct timeval *, struct timeval *);
__dead	void usage(void);
#if INET6
int	wait_for_reply(int, struct sockaddr *, struct timeval *);
#else /* INET6 */
int	wait_for_reply(int, struct sockaddr_in *, struct timeval *);
#endif /* INET6 */
int	get_src(struct sockaddr_in *, struct sockaddr_in *);

int
main(int argc, char **argv)
{
	register int op, code, n;
	register char *cp;
	register u_char *outp;
	register u_int32_t *ap;
#if !INET6
	register struct sockaddr_in *from = &wherefrom;
	register struct sockaddr_in *to = (struct sockaddr_in *)&whereto;
#endif /* !INET6 */
	register struct hostinfo *hi;
	int on = 1;
	register struct protoent *pe;
#if INET6
	int ttl, probe, i;
#else /* INET6 */
	register int ttl, probe, i;
#endif /* INET6 */
	register int seq = 0;
	int tos = 0, settos = 0;
	register int lsrr = 0;
	register u_short off = 0;
	struct ifaddrlist *al;
	char errbuf[132];

	if ((cp = strrchr(argv[0], '/')) != NULL)
		prog = cp + 1;
	else
		prog = argv[0];

	opterr = 0;
#if !INET6
	while ((op = getopt(argc, argv, "dFInrvxf:g:i:m:p:q:s:t:w:")) != EOF)
#else /* INET6 */
	while ((op = getopt(argc, argv, "dFInrvxf:g:i:m:p:q:s:t:w:a:")) != EOF)
#endif /* INET6 */
		switch (op) {
#if INET6
		case 'a':
			if (strcmp(optarg, "inet") == 0)
				i = AF_INET;
			else if (strcmp(optarg, "inet6") == 0)
				i = AF_INET6;
			else {
				fprintf(stderr, "%s: %s: invalid family\n",
					prog, optarg);
				exit(1);
			}
			to.sa.sa_family = i;
			break;
#endif /* INET6 */

		case 'd':
			options |= SO_DEBUG;
			break;

		case 'f':
			first_ttl = str2val(optarg, "first ttl", 1, 255);
			break;

		case 'F':
			off = IP_DF;
			break;

		case 'g':
			if (lsrr >= NGATEWAYS) {
				Fprintf(stderr,
				    "%s: No more than %d gateways\n",
				    prog, NGATEWAYS);
				exit(1);
			}
			getaddr(gwlist + lsrr, optarg);
			++lsrr;
			break;

		case 'i':
			device = optarg;
			break;

		case 'I':
			++useicmp;
			break;

		case 'm':
			max_ttl = str2val(optarg, "max ttl", 1, 255);
			break;

		case 'n':
			++nflag;
			break;

		case 'p':
			port = str2val(optarg, "port", 1, -1);
			break;

		case 'q':
			nprobes = str2val(optarg, "nprobes", 1, -1);
			break;

		case 'r':
			options |= SO_DONTROUTE;
			break;

		case 's':
			/*
			 * set the ip source address of the outbound
			 * probe (e.g., on a multi-homed host).
			 */
			source = optarg;
			break;

		case 't':
			tos = str2val(optarg, "tos", 0, 255);
			++settos;
			break;

		case 'v':
			++verbose;
			break;

		case 'x':
			docksum = (docksum == 0);
			break;

		case 'w':
			waittime = str2val(optarg, "wait time", 2, -1);
			break;

		default:
			usage();
		}

	if (first_ttl > max_ttl) {
		Fprintf(stderr,
		    "%s: first ttl (%d) may not be greater than max ttl (%d)\n",
		    prog, first_ttl, max_ttl);
		exit(1);
	}

	if (!docksum)
		Fprintf(stderr, "%s: Warning: ckecksums disabled\n", prog);

	if (lsrr > 0)
		optlen = (lsrr + 1) * sizeof(gwlist[0]);
#if !INET6
	minpacket = sizeof(*outip) + sizeof(*outdata) + optlen;
	if (useicmp)
		minpacket += 8;			/* XXX magic number */
	else
		minpacket += sizeof(*outudp);
	if (packlen == 0)
		packlen = minpacket;		/* minimum sized packet */
	else if (minpacket > packlen || packlen > maxpacket) {
		Fprintf(stderr, "%s: packet size must be %d <= s <= %d\n",
		    prog, minpacket, maxpacket);
		exit(1);
	}
#endif /* !INET6 */

	/* Process destination and optional packet size */
	switch (argc - optind) {

	case 2:
		packlen = str2val(argv[optind + 1],
#if INET6
		    "packet length", -1, -1);
#else /* INET6 */
		    "packet length", minpacket, -1);
#endif /* INET6 */
		/* Fall through */

	case 1:
#if INET6
	    {
		struct addrinfo req, *ai, *ai2;

		memset(&req, 0, sizeof(struct addrinfo));
		req.ai_family = to.sa.sa_family;

		if (i = getaddrinfo(hostname = argv[optind], NULL, &req, &ai)) {
			fprintf(stderr, "%s: %s: %s\n", prog,
			    hostname, gai_strerror(i));
			exit(1);
		}

		for (ai2 = ai; ai2 && (ai2->ai_family != AF_INET) &&
		    (ai2->ai_family != AF_INET6); ai2 = ai2->ai_next)
			;

		if (!ai2) {
			fprintf(stderr, "%s: %s: no IP version 4 or 6 "
			    "addresses available\n", prog, hostname);
			exit(1);
		}

		memcpy(&to, ai->ai_addr, ai->ai_addrlen);

		freeaddrinfo(ai);
	    }
#else /* INET6 */
		hostname = argv[optind];
		hi = gethostinfo(hostname);
		setsin(to, hi->addrs[0]);
		if (hi->n > 1)
			Fprintf(stderr,
		    "%s: Warning: %s has multiple addresses; using %s\n",
				prog, hostname, inet_ntoa(to->sin_addr));
		hostname = hi->name;
		hi->name = NULL;
		freehostinfo(hi);
#endif /* INET6 */
		break;

	default:
		usage();
	}

#ifdef HAVE_SETLINEBUF
	setlinebuf (stdout);
#else
	setvbuf(stdout, NULL, _IOLBF, 0);
#endif

#if INET6
	if (to.sa.sa_family == AF_INET) {
		if (useicmp)
			minpacket = sizeof(struct ip) + optlen +
			    ICMP_MINLEN + sizeof(struct outdata);
		else
			minpacket = sizeof(struct ip) + optlen +
			    sizeof(struct udphdr) + sizeof(struct outdata);
	} else {
		if (useicmp)
			minpacket = sizeof(struct ipv6hdr) +
			    sizeof(struct icmpv6hdr) + sizeof(struct outdata);
		else
			minpacket = sizeof(struct ipv6hdr) +
			    sizeof(struct udphdr) + sizeof(struct outdata);
	}
	if (!packlen)
		packlen = minpacket;
	if (minpacket > packlen || packlen > maxpacket) {
		fprintf(stderr, "%s: packet size must be %d <= s <= %d\n",
		    prog, minpacket, maxpacket);
		exit(1);
	}
#endif /* INET6 */

#if INET6
	if (!(outbuf = malloc(packlen))) {
		fprintf(stderr, "%s: malloc: %s\n", prog, strerror(errno));
		exit(1);
	}
	memset(outbuf, 0, packlen);

    if (to.sa.sa_family == AF_INET) {
	outip = (struct ip *)outbuf;
#else /* INET6 */
	outip = (struct ip *)malloc((unsigned)packlen);
	if (outip == NULL) {
		Fprintf(stderr, "%s: malloc: %s\n", prog, strerror(errno));
		exit(1);
	}
	memset((char *)outip, 0, packlen);
#endif /* INET6 */

	outip->ip_v = IPVERSION;
	if (settos)
		outip->ip_tos = tos;
#ifdef BYTESWAP_IP_LEN
	outip->ip_len = htons(packlen);
#else
	outip->ip_len = packlen;
#endif
	outip->ip_off = off;
	outp = (u_char *)(outip + 1);
#ifdef HAVE_RAW_OPTIONS
	if (lsrr > 0) {
		register u_char *optlist;

		optlist = outp;
		outp += optlen;

		/* final hop */
#if INET6
		gwlist[lsrr] = to.sin.sin_addr.s_addr;
#else /* INET6 */
		gwlist[lsrr] = to->sin_addr.s_addr;
#endif /* INET6 */

		outip->ip_dst.s_addr = gwlist[0];

		/* force 4 byte alignment */
		optlist[0] = IPOPT_NOP;
		/* loose source route option */
		optlist[1] = IPOPT_LSRR;
		i = lsrr * sizeof(gwlist[0]);
		optlist[2] = i + 3;
		/* Pointer to LSRR addresses */
		optlist[3] = IPOPT_MINOFF;
		memcpy(optlist + 4, gwlist + 1, i);
	} else
#endif
#if INET6
		outip->ip_dst = to.sin.sin_addr;
#else /* INET6 */
		outip->ip_dst = to->sin_addr;
#endif /* INET6 */

	outip->ip_hl = (outp - (u_char *)outip) >> 2;
	ident = (getpid() & 0xffff) | 0x8000;
	if (useicmp) {
		outip->ip_p = IPPROTO_ICMP;

		outicmp = (struct icmp *)outp;
		outicmp->icmp_type = ICMP_ECHO;
		outicmp->icmp_id = htons(ident);

		outdata = (struct outdata *)(outp + 8);	/* XXX magic number */
	} else {
		outip->ip_p = IPPROTO_UDP;

		outudp = (struct udphdr *)outp;
		outudp->uh_sport = htons(ident);
		outudp->uh_ulen =
		    htons((u_short)(packlen - (sizeof(*outip) + optlen)));
		outdata = (struct outdata *)(outudp + 1);
	}

	cp = "icmp";
#if INET6
    } else {
	ident = (getpid() & 0xffff) | 0x8000;
	if (useicmp) {
		struct icmpv6hdr *icmpv6 = (struct icmpv6hdr *)outbuf;
		memset(icmpv6, 0, sizeof(struct icmpv6hdr));
		icmpv6->icmpv6_type = ICMPV6_ECHO_REQUEST;
		icmpv6->icmpv6_id = htons(ident);

		outdata = (void *)outbuf + sizeof(struct icmpv6hdr);
	} else
		outdata = (void *)outbuf;
	cp = "icmpv6";
    }
#endif /* INET6 */
	if ((pe = getprotobyname(cp)) == NULL) {
		Fprintf(stderr, "%s: unknown protocol %s\n", prog, cp);
		exit(1);
	}
#if INET6
	if ((s = socket(to.sa.sa_family, SOCK_RAW, pe->p_proto)) < 0) {
		Fprintf(stderr, "%s: %s socket: %s\n", prog, cp, strerror(errno));
#else /* INET6 */
	if ((s = socket(AF_INET, SOCK_RAW, pe->p_proto)) < 0) {
		Fprintf(stderr, "%s: icmp socket: %s\n", prog, strerror(errno));
#endif /* INET6 */
		exit(1);
	}
	if (options & SO_DEBUG)
		(void)setsockopt(s, SOL_SOCKET, SO_DEBUG, (char *)&on,
		    sizeof(on));
	if (options & SO_DONTROUTE)
		(void)setsockopt(s, SOL_SOCKET, SO_DONTROUTE, (char *)&on,
		    sizeof(on));

#if INET6
	if (to.sa.sa_family == AF_INET6) {
#ifdef ICMPV6_FILTER_SETBLOCKALL
		struct icmpv6_filter filter;

		ICMPV6_FILTER_SETBLOCKALL(&filter);

		ICMPV6_FILTER_SETPASS(ICMPV6_DST_UNREACH, &filter);
		ICMPV6_FILTER_SETPASS(ICMPV6_TIME_EXCEEDED, &filter);
		ICMPV6_FILTER_SETPASS(ICMPV6_ECHO_REPLY, &filter);

		if (setsockopt(s, IPPROTO_ICMPV6, ICMPV6_FILTER, &filter,
		    sizeof(struct icmpv6_filter)) < 0) {
			perror(
			    "setsockopt(IPPROTO_ICMPV6, ICMPV6_FILTER, ...)");
			exit(1);
		}
#endif /* ICMPV6_FILTER_SETBLOCKALL */

		if (useicmp) {
			sndsock = socket(AF_INET6, SOCK_RAW, pe->p_proto);
			if (sndsock < 0) {
				fprintf(stderr, "%s: icmpv6 send socket: %s\n",
				    prog, strerror(errno));
				exit(1);
			}

			i = 2;
			if (setsockopt(sndsock, IPPROTO_IPV6, IPV6_CHECKSUM,
			    &i, sizeof(int)) < 0) {
				perror("setsockopt(IPPROTO_IPV6, "
				    "IPV6_CHECKSUM, ...)");
				exit(1);
			}
		} else {
			sndsock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
			if (sndsock < 0) {
				fprintf(stderr, "%s: udp socket: %s\n", prog,
				    strerror(errno));
				exit(1);
			}
		}
	} else
#endif /* INET6 */
#ifndef __hpux
	sndsock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
#else
	sndsock = socket(AF_INET, SOCK_RAW,
	    useicmp ? IPPROTO_ICMP : IPPROTO_UDP);
#endif
	if (sndsock < 0) {
		Fprintf(stderr, "%s: raw socket: %s\n", prog, strerror(errno));
		exit(1);
	}

	/* Revert to non-privileged user after opening sockets */
	setuid(getuid());

#if defined(IP_OPTIONS) && !defined(HAVE_RAW_OPTIONS)
#if INET6
	if (to.sa.sa_family == AF_INET)
#endif /* INET6 */
	if (lsrr > 0) {
		u_char optlist[MAX_IPOPTLEN];

		cp = "ip";
		if ((pe = getprotobyname(cp)) == NULL) {
			Fprintf(stderr, "%s: unknown protocol %s\n", prog, cp);
			exit(1);
		}

		/* final hop */
#if INET6
		gwlist[lsrr] = to.sin.sin_addr.s_addr;
#else /* INET6 */
		gwlist[lsrr] = to->sin_addr.s_addr;
#endif /* INET6 */
		++lsrr;

		/* force 4 byte alignment */
		optlist[0] = IPOPT_NOP;
		/* loose source route option */
		optlist[1] = IPOPT_LSRR;
		i = lsrr * sizeof(gwlist[0]);
		optlist[2] = i + 3;
		/* Pointer to LSRR addresses */
		optlist[3] = IPOPT_MINOFF;
		memcpy(optlist + 4, gwlist, i);

		if ((setsockopt(sndsock, pe->p_proto, IP_OPTIONS, optlist,
		    i + sizeof(gwlist[0]))) < 0) {
			Fprintf(stderr, "%s: IP_OPTIONS: %s\n",
			    prog, strerror(errno));
			exit(1);
		    }
	}
#endif

#ifdef SO_SNDBUF
	if (setsockopt(sndsock, SOL_SOCKET, SO_SNDBUF, (char *)&packlen,
	    sizeof(packlen)) < 0) {
		Fprintf(stderr, "%s: SO_SNDBUF: %s\n", prog, strerror(errno));
		exit(1);
	}
#endif
#ifdef IP_HDRINCL
#if INET6
	if (to.sa.sa_family == AF_INET)
#endif /* INET6 */
	if (setsockopt(sndsock, IPPROTO_IP, IP_HDRINCL, (char *)&on,
	    sizeof(on)) < 0) {
		Fprintf(stderr, "%s: IP_HDRINCL: %s\n", prog, strerror(errno));
		exit(1);
	}
#else
#ifdef IP_TOS
#if INET6
	if (to.sa.sa_family == AF_INET)
#endif /* INET6 */
	if (settos && setsockopt(sndsock, IPPROTO_IP, IP_TOS,
	    (char *)&tos, sizeof(tos)) < 0) {
		Fprintf(stderr, "%s: setsockopt tos %d: %s\n",
		    prog, tos, strerror(errno));
		exit(1);
	}
#endif
#endif
	if (options & SO_DEBUG)
		(void)setsockopt(sndsock, SOL_SOCKET, SO_DEBUG, (char *)&on,
		    sizeof(on));
	if (options & SO_DONTROUTE)
		(void)setsockopt(sndsock, SOL_SOCKET, SO_DONTROUTE, (char *)&on,
		    sizeof(on));

#if INET6
    if (to.sa.sa_family == AF_INET) {
#endif /* INET6 */
	/* Get the interface address list */
	n = ifaddrlist(&al, errbuf);
	if (n < 0) {
		Fprintf(stderr, "%s: ifaddrlist: %s\n", prog, errbuf);
		exit(1);
	}
	if (n == 0) {
		Fprintf(stderr,
		    "%s: Can't find any network interfaces\n", prog);
		exit(1);
	}

	/* Look for a specific device */
	if (device != NULL) {
		for (i = n; i > 0; --i, ++al)
			if (strcmp(device, al->device) == 0)
				break;
		if (i <= 0) {
			Fprintf(stderr, "%s: Can't find interface %s\n",
			    prog, device);
			exit(1);
		}
	}
#if INET6
    }
#endif /* INET6 */

	/* Determine our source address */
	if (source == NULL) {
#if INET6
		memset(&from, 0, sizeof(from));

		if (to.sa.sa_family == AF_INET &&
		    (device || !get_src(&to.sin, &from.sin))) {
			setsin(&from.sin, al->addr);
			if (n > 1 && device == NULL)
				Fprintf(stderr, "%s: Warning: Multiple "
				    "interfaces found; using %s @ %s\n", prog,
				    inet_ntoa(from.sin.sin_addr), al->device);
		}
#else /* INET6 */
		/*
		 * If a device was specified, use the interface address.
		 * Otherwise, ask the kernel which src address to use.
		 * If that fails, use the first interface found and
		 * warn if there are more than one.
		 */

		if (device)
			setsin(from, al->addr);
		else if (!get_src(to, from)) {
			setsin(from, al->addr);
			if (n > 1) {
				Fprintf(stderr, "%s: Warning: Multiple "
				    "interfaces found; using %s @ %s\n", prog,
				    inet_ntoa(from->sin_addr), al->device);
			}
		}
#endif /* INET6 */
	} else {
#if INET6
		struct addrinfo req, *ai;

		memset(&req, 0, sizeof(struct addrinfo));
		req.ai_family = to.sa.sa_family;

		if (i = getaddrinfo(source, NULL, &req, &ai)) {
			fprintf(stderr, "%s: %s: %s\n", prog, source,
			    gai_strerror(i));
			exit(1);
		}

		memcpy(&from, ai->ai_addr, ai->ai_addrlen);

#define	ISONLOOPBACKNET(x)	((ntohl(x) >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET)
		switch (from.sa.sa_family) {
		case AF_INET:
			if (ISONLOOPBACKNET(from.sin.sin_addr.s_addr)) {
			    bad_source:
				fprintf(stderr,
				    "%s: source address invalid: %s\n", prog,
				    source);
				exit(1);
			}
			break;

		case AF_INET6:
			if (IN6_IS_ADDR_LOOPBACK(&from.sin6.sin6_addr) ||
			    (IN6_IS_ADDR_V4MAPPED(&from.sin6.sin6_addr) &&
			     ISONLOOPBACKNET(*(long *)(&from.sin6.sin6_addr.s6_addr[12]))))
				goto bad_source;
			break;

		default:
			goto bad_source;
		}

		freeaddrinfo(ai);
	}
#else /* INET6 */
		hi = gethostinfo(source);
		source = hi->name;
		hi->name = NULL;
		if (device == NULL) {
			/*
			 * Use the first interface found.
			 * Warn if there are more than one.
			 */
			setsin(from, hi->addrs[0]);
			if (hi->n > 1)
				Fprintf(stderr,
			"%s: Warning: %s has multiple addresses; using %s\n",
				    prog, source, inet_ntoa(from->sin_addr));
		} else {
			/*
			 * Make sure the source specified matches the
			 * interface address.
			 */
			for (i = hi->n, ap = hi->addrs; i > 0; --i, ++ap)
				if (*ap == al->addr)
					break;
			if (i <= 0) {
				Fprintf(stderr,
				    "%s: %s is not on interface %s\n",
				    prog, source, device);
				exit(1);
			}
			setsin(from, *ap);
		}
		freehostinfo(hi);
	}
	outip->ip_src = from->sin_addr;
#ifndef IP_HDRINCL
	if (bind(sndsock, (struct sockaddr *)from, sizeof(*from)) < 0) { Fprintf(stderr, "%s: bind: %s\n",
		    prog, strerror(errno));
		exit (1);
	}
#endif
#endif /* INET6 */

#if INET6
    {
	char abuf[NI_MAXHOST];

	if (getnameinfo((struct sockaddr *)&to, to.sa.sa_len, abuf,
	    sizeof(abuf), NULL, 0, NI_NUMERICHOST)) {
		fprintf(stderr, "%s: 1getnameinfo failed\n", prog);
		exit(1);
	}
	fprintf(stderr, "%s to %s (%s)", prog, hostname, abuf);
	if (from.sa.sa_family) {
		fprintf(stderr, " from ");
		if (source) {
			fprintf(stderr, source);

			if (getnameinfo((struct sockaddr *)&from,
			    from.sa.sa_len, abuf, sizeof(abuf), NULL,
			    0, NI_NUMERICHOST)) {
				fprintf(stderr, "%s: 2getnameinfo failed\n",
				    prog);
				exit(1);
			}
			fprintf(stderr, " (%s)", abuf);
		} else if (nflag) {
			if (getnameinfo((struct sockaddr *)&from,
			    from.sa.sa_len, abuf, sizeof(abuf),
			    NULL, 0, NI_NUMERICHOST)) {
				fprintf(stderr,
				    "%s: 3getnameinfo failed\n", prog);
				exit(1);
			}
			fprintf(stderr, abuf);
		} else {
			if (i = getnameinfo((struct sockaddr *)&from,
			    from.sa.sa_len, abuf, sizeof(abuf),
			    NULL, 0, 0)) {
				fprintf(stderr,
				    "%s: 4getnameinfo failed %d\n", prog, i);
				exit(1);
			}
			fprintf(stderr, abuf);

			if (getnameinfo((struct sockaddr *)&from,
			    from.sa.sa_len, abuf, sizeof(abuf),
			    NULL, 0, NI_NUMERICHOST)) {
				fprintf(stderr,
				    "%s: 5getnameinfo failed\n", prog);
				exit(1);
			}
			fprintf(stderr, " (%s)", abuf);
		}
	}
    }

	if (to.sa.sa_family == AF_INET)
		outip->ip_src = from.sin.sin_addr;

	if (!from.sa.sa_family) {
#ifdef HAVE_SOCKADDR_SA_LEN
		from.sa.sa_len = to.sa.sa_len;
#endif
		from.sa.sa_family = to.sa.sa_family;
	}

	if (bind(sndsock, (struct sockaddr *)&from, from.sa.sa_len) < 0) {
		fprintf(stderr, "%s: bind: %s\n", prog, strerror(errno));
		exit(1);
	}

	i = sizeof(su);
	if (getsockname(sndsock, (struct sockaddr *)&from, &i) < 0) {
		fprintf(stderr, "%s: getsockname: %s\n", prog, strerror(errno));
		exit(1);
	}

	if (to.sa.sa_family == AF_INET6)
		sport = from.sin6.sin6_port;
#else /* INET6 */
	Fprintf(stderr, "%s to %s (%s)",
	    prog, hostname, inet_ntoa(to->sin_addr));
	if (source)
		Fprintf(stderr, " from %s (%s)",
		    source, inet_ntoa(from->sin_addr));
	else
		Fprintf(stderr, " from %s", inet_ntoa(from->sin_addr));
#endif /* INET6 */
	Fprintf(stderr, ", %d hops max, %d byte packets\n", max_ttl, packlen);
	(void)fflush(stderr);

#if INET6
	memset(&sendmsghdr, 0, sizeof(struct msghdr));
	sendmsghdr.msg_name = (caddr_t)&to;
	sendmsghdr.msg_namelen = to.sa.sa_len;
	sendmsghdr.msg_iov = &sendiovec;
	sendmsghdr.msg_iovlen = 1;

	if (to.sa.sa_family == AF_INET6) {
		struct cmsghdr *cmsghdr;
		struct in6_pktinfo *in6_pktinfo;

		sendmsghdr.msg_control = sendcontrol;
		sendmsghdr.msg_controllen = CMSG_SPACE(sizeof(int));

		memset(sendcontrol, 0, sizeof(sendcontrol));

		cmsghdr = (struct cmsghdr *)sendcontrol;
		cmsghdr->cmsg_len = CMSG_LEN(sizeof(int));
		cmsghdr->cmsg_level = IPPROTO_IPV6;
		cmsghdr->cmsg_type = IPV6_HOPLIMIT;

		if (device || source) {
			cmsghdr = (void *)cmsghdr + CMSG_SPACE(sizeof(int));
			cmsghdr->cmsg_len =
			    CMSG_LEN(sizeof(struct in6_pktinfo));
			cmsghdr->cmsg_level = IPPROTO_IPV6;
			cmsghdr->cmsg_type = IPV6_PKTINFO;

			in6_pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsghdr);
			if (device)
				if (!(in6_pktinfo->ipi6_ifindex =
				    if_nametoindex(device))) {
					fprintf(stderr,
					    "%s: invalid device: %s\n",
					    prog, device);
					exit(1);
				}

			if (source)
				in6_pktinfo->ipi6_addr = from.sin6.sin6_addr;
			sendmsghdr.msg_controllen +=
			    CMSG_SPACE(sizeof(struct in6_pktinfo));
		}
	}

	memset(&sendiovec, 0, sizeof(struct iovec));
	sendiovec.iov_base = (caddr_t)outbuf;
	if (to.sa.sa_family == AF_INET6) {
		if (useicmp)
			sendiovec.iov_len = packlen - sizeof(struct udphdr) -
			    sizeof(struct ipv6hdr);
		else
			sendiovec.iov_len = packlen - sizeof(struct ipv6hdr);
	} else
		sendiovec.iov_len = packlen;

	memset(&recvmsghdr, 0, sizeof(struct msghdr));
	recvmsghdr.msg_name = (caddr_t)&from;
	recvmsghdr.msg_iov = &recviovec;
	recvmsghdr.msg_iovlen = 1;
	if (to.sa.sa_family == AF_INET6)
		recvmsghdr.msg_control = recvcontrol;

	memset(&recviovec, 0, sizeof(struct iovec));
	recviovec.iov_base = (caddr_t)packet;
	recviovec.iov_len = sizeof(packet);
#endif /* INET6 */

	for (ttl = first_ttl; ttl <= max_ttl; ++ttl) {
#if !INET6
		u_int32_t lastaddr = 0;
#endif /* INET6 */
		int got_there = 0;
		int unreachable = 0;

#if INET6
		memset(&lastaddr, 0, sizeof(su));
#endif /* INET6 */
		Printf("%2d ", ttl);

		for (probe = 0; probe < nprobes; ++probe) {
			register int cc;
			struct timeval t1, t2;
			struct timezone tz;
			register struct ip *ip;

			(void)gettimeofday(&t1, &tz);
			send_probe(++seq, ttl, &t1);
#if INET6
			recvmsghdr.msg_namelen = sizeof(su);
			if (recvmsghdr.msg_control)
				recvmsghdr.msg_controllen = sizeof(recvcontrol);

			while ((cc = wait_for_reply(s,
			    (struct sockaddr *)&from, &t1)) != 0) {
#else /* INET6 */
			while ((cc = wait_for_reply(s, from, &t1)) != 0) {
#endif /* INET6 */
				(void)gettimeofday(&t2, &tz);
#if INET6
				if (to.sa.sa_family == AF_INET6)
					i = packet_ok6(packet, cc,
					    (struct sockaddr *)&from, seq);
				else
					i = packet_ok(packet, cc,
					    (struct sockaddr_in *)&from, seq);
#else /* INET6 */
				i = packet_ok(packet, cc, from, seq);
#endif /* INET6 */
				/* Skip short packet */
				if (i == 0)
					continue;
#if INET6
				if (to.sa.sa_family == AF_INET) {
					from.sin.sin_port = 0;
					memset(&from.sin.sin_zero, 0,
					    sizeof(from.sin.sin_zero));
				} else {
					from.sin6.sin6_port = 0;
					from.sin6.sin6_flowinfo = 0;
				}

				if (memcmp(&from, &lastaddr, from.sa.sa_len)) {
					memcpy(&lastaddr, &from,
					    from.sa.sa_len);
					print(packet, cc,
					    (struct sockaddr *)&from);
				}
#else /* INET6 */
				if (from->sin_addr.s_addr != lastaddr) {
					print(packet, cc, from);
					lastaddr = from->sin_addr.s_addr;
				}
#endif /* INET6 */
				Printf("  %.3f ms", deltaT(&t1, &t2));
				if (i == -2) {
#ifndef ARCHAIC
					ip = (struct ip *)packet;
					if (ip->ip_ttl <= 1)
						Printf(" !");
#endif
					++got_there;
					break;
				}
				/* time exceeded in transit */
				if (i == -1)
					break;
				code = i - 1;
				switch (code) {

				case ICMP_UNREACH_PORT:
#if INET6
				    if (to.sa.sa_family == AF_INET6) {
					struct cmsghdr *cmsghdr;
					for (cmsghdr = CMSG_FIRSTHDR(
					    &recvmsghdr); cmsghdr; cmsghdr =
					    CMSG_NXTHDR(&recvmsghdr, cmsghdr))
						if ((cmsghdr->cmsg_level ==
						    IPPROTO_IPV6) &&
						    (cmsghdr->cmsg_type ==
						    IPV6_HOPLIMIT) &&
						    (cmsghdr->cmsg_len ==
						    sizeof(struct cmsghdr) +
						    sizeof(int)) &&
						    (*(int *)CMSG_DATA(cmsghdr)
						    >= 0) &&
						    (*(int *)CMSG_DATA(cmsghdr)
						    < 256))
							if (*(int *)CMSG_DATA(
							    cmsghdr) <= 1)
								printf(" !");
				    } else {
#endif /* INET6 */
#ifndef ARCHAIC
					ip = (struct ip *)packet;
					if (ip->ip_ttl <= 1)
						Printf(" !");
#endif
#if INET6
				    }
#endif /* INET6 */
					++got_there;
					break;

				case ICMP_UNREACH_NET:
					++unreachable;
					Printf(" !N");
					break;

				case ICMP_UNREACH_HOST:
					++unreachable;
					Printf(" !H");
					break;

				case ICMP_UNREACH_PROTOCOL:
					++got_there;
					Printf(" !P");
					break;

				case ICMP_UNREACH_NEEDFRAG:
					++unreachable;
					Printf(" !F");
					break;

				case ICMP_UNREACH_SRCFAIL:
					++unreachable;
					Printf(" !S");
					break;

/* rfc1716 */
#ifndef ICMP_UNREACH_FILTER_PROHIB
#define ICMP_UNREACH_FILTER_PROHIB	13	/* admin prohibited filter */
#endif
				case ICMP_UNREACH_FILTER_PROHIB:
					++unreachable;
					Printf(" !X");
					break;

				default:
					++unreachable;
					Printf(" !<%d>", code);
					break;
				}
				break;
			}
			if (cc == 0)
				Printf(" *");
			(void)fflush(stdout);
		}
		putchar('\n');
		if (got_there ||
		    (unreachable > 0 && unreachable >= nprobes - 1))
			break;
	}
	exit(0);
}

int
#if INET6
wait_for_reply(int sock, struct sockaddr *fromp, struct timeval *tp)
#else /* INET6 */
wait_for_reply(register int sock, register struct sockaddr_in *fromp,
    register struct timeval *tp)
#endif /* INET6 */
{
	fd_set fds;
	struct timeval now, wait;
	struct timezone tz;
	register int cc = 0;
#if !INET6
	int fromlen = sizeof(*fromp);
#endif /* !INET6 */

	FD_ZERO(&fds);
	FD_SET(sock, &fds);

	wait.tv_sec = tp->tv_sec + waittime;
	wait.tv_usec = tp->tv_usec;
	(void)gettimeofday(&now, &tz);
	if (wait.tv_sec < now.tv_sec + 2)
		wait.tv_sec = now.tv_sec + 2;
	tvsub(&wait, &now);

	if (select(sock + 1, &fds, NULL, NULL, &wait) > 0)
#if INET6
		cc = recvmsg(sock, &recvmsghdr, 0);
#else /* INET6 */
		cc = recvfrom(s, (char *)packet, sizeof(packet), 0,
			    (struct sockaddr *)fromp, &fromlen);
#endif /* INET6 */

	return(cc);
}

#if INET6
void
send_probe(register int seq, register int ttl, register struct timeval *tp)
{
	int i;

	/* Payload */
	outdata->seq = seq;
	outdata->ttl = ttl;
	outdata->tv = *tp;

	if (to.sa.sa_family == AF_INET6) {
		*((int *)CMSG_DATA((struct cmsghdr *)sendcontrol)) = ttl;

		if (useicmp) {
			struct icmpv6hdr *icmpv6 = (struct icmpv6hdr *)outbuf;
			icmpv6->icmpv6_seq = htons(seq);
		} else
			to.sin6.sin6_port = htons(port + seq);
	} else {
		outip->ip_ttl = ttl;
		outip->ip_id = htons(ident + seq);

		/*
		 * In most cases, the kernel will recalculate the ip checksum.
		 * But we must do it anyway so that the udp checksum comes out
		 * right.
		 */
		if (docksum) {
			outip->ip_sum = in_cksum((u_short *)outip,
			    sizeof(*outip) + optlen);
			if (outip->ip_sum == 0)
				outip->ip_sum = 0xffff;
		}

		if (useicmp)
			outicmp->icmp_seq = htons(seq);
		else
			outudp->uh_dport = htons(port + seq);

		if (docksum) {
			if (useicmp) {
				outicmp->icmp_cksum = 0;
				outicmp->icmp_cksum =
				    in_cksum((u_short *)outicmp,
				    packlen - (sizeof(*outip) + optlen));
				if (outicmp->icmp_cksum == 0)
					outicmp->icmp_cksum = 0xffff;
			} else {
				struct ip tip;
				register struct udpiphdr * ui;

				/* Checksum (must save and restore ip header) */
				tip = *outip;
				ui = (struct udpiphdr *)outip;
#ifndef __bsdi__
				ui->ui_next = 0;
				ui->ui_prev = 0;
#else
				ui->ui_x00 = 0;
				ui->ui_x01 = 0;
#endif
				ui->ui_x1 = 0;
				ui->ui_len = outudp->uh_ulen;
				outudp->uh_sum = 0;
				outudp->uh_sum = in_cksum((u_short *)ui,
				    packlen);
				if (outudp->uh_sum == 0)
					outudp->uh_sum = 0xffff;
				*outip = tip;
			}
		}

		if (!useicmp)
			to.sin.sin_port = htons(port + seq);
	}

	if ((i = sendmsg(sndsock, &sendmsghdr, 0)) != sendiovec.iov_len) {
		if (i < 0) {
			if (errno == ECONNREFUSED)
				return;
			fprintf(stderr, "%s: sendmsg: %s\n",
			    prog, strerror(errno));
		}
		printf("%s: wrote %s %d chars, ret=%d\n",
		    prog, hostname, sendiovec.iov_len, i);
		fflush(stdout);
	}
}
#else /* INET6 */
void
send_probe(register int seq, int ttl, register struct timeval *tp)
{
	register int cc;
	register struct udpiphdr * ui;
	struct ip tip;

	outip->ip_ttl = ttl;
#ifndef __hpux
	outip->ip_id = htons(ident + seq);
#endif

	/*
	 * In most cases, the kernel will recalculate the ip checksum.
	 * But we must do it anyway so that the udp checksum comes out
	 * right.
	 */
	if (docksum) {
		outip->ip_sum =
		    in_cksum((u_short *)outip, sizeof(*outip) + optlen);
		if (outip->ip_sum == 0)
			outip->ip_sum = 0xffff;
	}

	/* Payload */
	outdata->seq = seq;
	outdata->ttl = ttl;
	outdata->tv = *tp;

	if (useicmp)
		outicmp->icmp_seq = htons(seq);
	else
		outudp->uh_dport = htons(port + seq);

	/* (We can only do the checksum if we know our ip address) */
	if (docksum) {
		if (useicmp) {
			outicmp->icmp_cksum = 0;
			outicmp->icmp_cksum = in_cksum((u_short *)outicmp,
			    packlen - (sizeof(*outip) + optlen));
			if (outicmp->icmp_cksum == 0)
				outicmp->icmp_cksum = 0xffff;
		} else {
			/* Checksum (must save and restore ip header) */
			tip = *outip;
			ui = (struct udpiphdr *)outip;
#ifndef __bsdi
			ui->ui_next = 0;
			ui->ui_prev = 0;
#else
			ui->ui_x00 = 0;
			ui->ui_x01 = 0;
#endif
			ui->ui_x1 = 0;
			ui->ui_len = outudp->uh_ulen;
			outudp->uh_sum = 0;
			outudp->uh_sum = in_cksum((u_short *)ui, packlen);
			if (outudp->uh_sum == 0)
				outudp->uh_sum = 0xffff;
			*outip = tip;
		}
	}

	/* XXX undocumented debugging hack */
	if (verbose > 1) {
		register const u_short *sp;
		register int nshorts, i;

		sp = (u_short *)outip;
		nshorts = (u_int)packlen / sizeof(u_short);
		i = 0;
		Printf("[ %d bytes", packlen);
		while (--nshorts >= 0) {
			if ((i++ % 8) == 0)
				Printf("\n\t");
			Printf(" %04x", ntohs(*sp++));
		}
		if (packlen & 1) {
			if ((i % 8) == 0)
				Printf("\n\t");
			Printf(" %02x", *(u_char *)sp);
		}
		Printf("]\n");
	}

#if !defined(IP_HDRINCL) && defined(IP_TTL)
	if (setsockopt(sndsock, IPPROTO_IP, IP_TTL,
	    (char *)&ttl, sizeof(ttl)) < 0) {
		Fprintf(stderr, "%s: setsockopt ttl %d: %s\n",
		    prog, ttl, strerror(errno));
		exit(1);
	}
#endif

#ifdef __hpux
	cc = sendto(sndsock, useicmp ? (char *)outicmp : (char *)outudp,
	    packlen - (sizeof(*outip) + optlen), 0, &whereto, sizeof(whereto));
	if (cc > 0)
		cc += sizeof(*outip) + optlen;
#else
	cc = sendto(sndsock, (char *)outip,
	    packlen, 0, &whereto, sizeof(whereto));
#endif
	if (cc < 0 || cc != packlen)  {
		if (cc < 0)
			Fprintf(stderr, "%s: sendto: %s\n",
			    prog, strerror(errno));
		Printf("%s: wrote %s %d chars, ret=%d\n",
		    prog, hostname, packlen, cc);
		(void)fflush(stdout);
	}
}
#endif /* INET6 */

double
deltaT(struct timeval *t1p, struct timeval *t2p)
{
	register double dt;

	dt = (double)(t2p->tv_sec - t1p->tv_sec) * 1000.0 +
	     (double)(t2p->tv_usec - t1p->tv_usec) / 1000.0;
	return (dt);
}

#if INET6
int
packet_ok6(u_char *buf, int cc, struct sockaddr *from, int seq)
{
	struct icmpv6hdr *icmpv6 = (struct icmpv6hdr *)buf;
	struct ipv6hdr *ipv6 =
	    (struct ipv6hdr *)(buf + sizeof(struct icmpv6hdr));

	switch(icmpv6->icmpv6_type) {
	case ICMPV6_TIME_EXCEEDED:
		if (icmpv6->icmpv6_code != ICMPV6_EXCEEDED_HOPS)
			break;
		if (useicmp) {
			struct icmpv6hdr *icmpv62 = (struct icmpv6hdr *)(buf +
			    sizeof(struct icmpv6hdr) + sizeof(struct ipv6hdr));
			if (cc < sizeof(struct icmpv6hdr) +
			    sizeof(struct ipv6hdr) + sizeof(struct icmpv6hdr))
				break;
			if (ipv6->ipv6_nextheader != IPPROTO_ICMPV6)
				break;
			if ((icmpv62->icmpv6_id != htons(ident)) ||
			    (icmpv62->icmpv6_seq != htons(seq)))
				break;
		} else {
			struct udphdr *udp = (struct udphdr *)(buf +
			    sizeof(struct icmpv6hdr) + sizeof(struct ipv6hdr));
			if (cc < sizeof(struct icmpv6hdr) +
			    sizeof(struct ipv6hdr) + sizeof(struct udphdr))
				break;
			if (ipv6->ipv6_nextheader != IPPROTO_UDP)
				break;
			if ((udp->uh_sport != sport) ||
			    (udp->uh_dport != htons(port + seq)))
			break;
		}
		return -1;
	case ICMPV6_DST_UNREACH:
		switch(icmpv6->icmpv6_code) {
		case ICMPV6_UNREACH_NOROUTE:
			return ICMP_UNREACH_NET + 1;
		case ICMPV6_UNREACH_ADMIN:
			return ICMP_UNREACH_FILTER_PROHIB + 1;
		case ICMPV6_UNREACH_NOTNEIGHBOR:
			break;
		case ICMPV6_UNREACH_ADDRESS:
			return ICMP_UNREACH_HOST + 1;
		case ICMPV6_UNREACH_PORT:
			return ICMP_UNREACH_PORT + 1;
		}
		break;
	case ICMPV6_ECHO_REPLY:
		if ((icmpv6->icmpv6_id == htons(ident)) ||
		    (icmpv6->icmpv6_seq == htons(seq)))
			return -2;
		break;
	}

	if (verbose) {
		int i;
		u_int32_t *lp = (u_int32_t *)(buf + sizeof(struct icmpv6hdr));
		char hbuf[64];

		if (getnameinfo(from, from->sa_len, hbuf, sizeof(hbuf),
		    NULL, 0, nflag ? NI_NUMERICHOST : 0)) {
			fprintf(stderr, "6getnameinfo failed!\n");
			exit(1);
		}

		printf("\n%d bytes from %s to ", cc, hbuf);

		if (cc >= sizeof(struct icmpv6hdr) + sizeof(struct ipv6hdr)) {
			struct sockaddr_in6 sin6;

			memset(&sin6, 0, sizeof(struct sockaddr_in6));

#ifdef HAVE_SOCKADDR_SA_LEN
			sin6.sin6_len = sizeof(struct sockaddr_in6);
#endif
			sin6.sin6_family = AF_INET6;
			sin6.sin6_addr = ipv6->ipv6_dst;

			if (getnameinfo((struct sockaddr *)&sin6,
			    sizeof(struct sockaddr_in6), hbuf, sizeof(hbuf),
			    NULL, 0, nflag ? NI_NUMERICHOST : 0)) {
				fprintf(stderr, "7getnameinfo failed!\n");
				exit(1);
			}

			printf("%s: icmp type %d code %d\n", hbuf,
			    icmpv6->icmpv6_type, icmpv6->icmpv6_code);
		}

		for (i = sizeof(struct icmpv6hdr); i < cc ; i += sizeof(*lp))
			printf("%2d: x%8.8x\n", i, *lp++);
	}
	return(0);
}
#endif /* INET6 */

/*
 * Convert an ICMP "type" field to a printable string.
 */
char *
pr_type(register u_char t)
{
	static char *ttab[] = {
	"Echo Reply",	"ICMP 1",	"ICMP 2",	"Dest Unreachable",
	"Source Quench", "Redirect",	"ICMP 6",	"ICMP 7",
	"Echo",		"ICMP 9",	"ICMP 10",	"Time Exceeded",
	"Param Problem", "Timestamp",	"Timestamp Reply", "Info Request",
	"Info Reply"
	};

	if (t > 16)
		return("OUT-OF-RANGE");

	return(ttab[t]);
}

int
packet_ok(register u_char *buf, int cc, register struct sockaddr_in *from,
    register int seq)
{
	register struct icmp *icp;
	register u_char type, code;
	register int hlen;
#ifndef ARCHAIC
	register struct ip *ip;

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {
		if (verbose)
			Printf("packet too short (%d bytes) from %s\n", cc,
				inet_ntoa(from->sin_addr));
		return (0);
	}
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
#else
	icp = (struct icmp *)buf;
#endif
	type = icp->icmp_type;
	code = icp->icmp_code;
	if ((type == ICMP_TIMXCEED && code == ICMP_TIMXCEED_INTRANS) ||
	    type == ICMP_UNREACH || type == ICMP_ECHOREPLY) {
		register struct ip *hip;
		register struct udphdr *up;
		register struct icmp *hicmp;

		hip = &icp->icmp_ip;
		hlen = hip->ip_hl << 2;
		if (useicmp) {
			/* XXX */
			if (type == ICMP_ECHOREPLY &&
			    icp->icmp_id == htons(ident) &&
			    icp->icmp_seq == htons(seq))
				return (-2);

			hicmp = (struct icmp *)((u_char *)hip + hlen);
			/* XXX 8 is a magic number */
			if (hlen + 8 <= cc &&
			    hip->ip_p == IPPROTO_ICMP &&
			    hicmp->icmp_id == htons(ident) &&
			    hicmp->icmp_seq == htons(seq))
				return (type == ICMP_TIMXCEED ? -1 : code + 1);
		} else {
			up = (struct udphdr *)((u_char *)hip + hlen);
			/* XXX 8 is a magic number */
			if (hlen + 12 <= cc &&
			    hip->ip_p == IPPROTO_UDP &&
			    up->uh_sport == htons(ident) &&
			    up->uh_dport == htons(port + seq))
				return (type == ICMP_TIMXCEED ? -1 : code + 1);
		}
	}
#ifndef ARCHAIC
	if (verbose) {
		register int i;
		u_int32_t *lp = (u_int32_t *)&icp->icmp_ip;

		Printf("\n%d bytes from %s to ", cc, inet_ntoa(from->sin_addr));
		Printf("%s: icmp type %d (%s) code %d\n",
		    inet_ntoa(ip->ip_dst), type, pr_type(type), icp->icmp_code);
		for (i = 4; i < cc ; i += sizeof(*lp))
			Printf("%2d: x%8.8x\n", i, *lp++);
	}
#endif
	return(0);
}


void
#if INET6
print(register u_char *buf, register int cc, register struct sockaddr *from)
#else /* INET6 */
print(register u_char *buf, register int cc, register struct sockaddr_in *from)
#endif /* INET6 */
{
#if INET6
	char hbuf[64], abuf[46];
	su dst;

	if (getnameinfo(from, from->sa_len, abuf, sizeof(abuf),
	    NULL, 0, NI_NUMERICHOST)) {
		fprintf(stderr, "8getnameinfo failed!\n");
		exit(1);
	}

	if (nflag)
		printf(" %s", abuf);
	else if (getnameinfo(from, from->sa_len, hbuf, sizeof(hbuf),
	    NULL, 0, 0)) {
		fprintf(stderr, "9getnameinfo failed!\n");
		exit(1);
	} else
		printf(" %s (%s)", hbuf, abuf);

	if (!verbose)
		return;

	memcpy(&dst, from, from->sa_len);

	if (to.sa.sa_family == AF_INET6)
		dst.sin6.sin6_addr = ((struct ipv6hdr *)(buf +
		    sizeof(struct icmpv6hdr)))->ipv6_dst;
	else
		dst.sin.sin_addr = ((struct ip *)buf)->ip_dst;

	if (getnameinfo(from, from->sa_len, hbuf, sizeof(hbuf), NULL, 0, 0)) {
		fprintf(stderr, "10getnameinfo failed!\n");
		exit(1);
	}

	printf(" %d bytes to %s", cc, hbuf);
#else /* INET6 */
	register struct ip *ip;
	register int hlen;

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	cc -= hlen;

	if (nflag)
		Printf(" %s", inet_ntoa(from->sin_addr));
	else
		Printf(" %s (%s)", inetname(from->sin_addr),
		    inet_ntoa(from->sin_addr));

	if (verbose)
		Printf(" %d bytes to %s", cc, inet_ntoa (ip->ip_dst));
#endif /* INET6 */
}

/*
 * Checksum routine for Internet Protocol family headers (C Version)
 */
u_short
in_cksum(register u_short *addr, register int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	register int sum = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1)
		sum += *(u_char *)w;

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

/*
 * Subtract 2 timeval structs:  out = out - in.
 * Out is assumed to be >= in.
 */
void
tvsub(register struct timeval *out, register struct timeval *in)
{

	if ((out->tv_usec -= in->tv_usec) < 0)   {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

/*
 * Construct an Internet address representation.
 * If the nflag has been supplied, give
 * numeric value, otherwise try for symbolic name.
 */
char *
inetname(struct in_addr in)
{
	register char *cp;
	register struct hostent *hp;
	static int first = 1;
	static char domain[MAXHOSTNAMELEN + 1], line[MAXHOSTNAMELEN + 1];

	if (first && !nflag) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = strchr(domain, '.')) != NULL) {
			(void)strncpy(domain, cp + 1, sizeof(domain) - 1);
			domain[sizeof(domain) - 1] = '\0';
		} else
			domain[0] = '\0';
	}
	if (!nflag && in.s_addr != INADDR_ANY) {
		hp = gethostbyaddr((char *)&in, sizeof(in), AF_INET);
		if (hp != NULL) {
			if ((cp = strchr(hp->h_name, '.')) != NULL &&
			    strcmp(cp + 1, domain) == 0)
				*cp = '\0';
			(void)strncpy(line, hp->h_name, sizeof(line) - 1);
			line[sizeof(line) - 1] = '\0';
			return (line);
		}
	}
	return (inet_ntoa(in));
}

struct hostinfo *
gethostinfo(register char *hostname)
{
	register int n;
	register struct hostent *hp;
	register struct hostinfo *hi;
	register char **p;
	register u_int32_t addr, *ap;

	hi = calloc(1, sizeof(*hi));
	if (hi == NULL) {
		Fprintf(stderr, "%s: calloc %s\n", prog, strerror(errno));
		exit(1);
	}
	addr = inet_addr(hostname);
	if ((int32_t)addr != -1) {
		hi->name = strdup(hostname);
		hi->n = 1;
		hi->addrs = calloc(1, sizeof(hi->addrs[0]));
		if (hi->addrs == NULL) {
			Fprintf(stderr, "%s: calloc %s\n",
			    prog, strerror(errno));
			exit(1);
		}
		hi->addrs[0] = addr;
		return (hi);
	}

	hp = gethostbyname(hostname);
	if (hp == NULL) {
		Fprintf(stderr, "%s: unknown host %s\n", prog, hostname);
		exit(1);
	}
	if (hp->h_addrtype != AF_INET || hp->h_length != 4) {
		Fprintf(stderr, "%s: bad host %s\n", prog, hostname);
		exit(1);
	}
	hi->name = strdup(hp->h_name);
	for (n = 0, p = hp->h_addr_list; *p != NULL; ++n, ++p)
		continue;
	hi->n = n;
	hi->addrs = calloc(n, sizeof(hi->addrs[0]));
	if (hi->addrs == NULL) {
		Fprintf(stderr, "%s: calloc %s\n", prog, strerror(errno));
		exit(1);
	}
	for (ap = hi->addrs, p = hp->h_addr_list; *p != NULL; ++ap, ++p)
		memcpy(ap, *p, sizeof(*ap));
	return (hi);
}

void
freehostinfo(register struct hostinfo *hi)
{
	if (hi->name != NULL) {
		free(hi->name);
		hi->name = NULL;
	}
	free((char *)hi->addrs);
	free((char *)hi);
}

void
getaddr(register u_int32_t *ap, register char *hostname)
{
	register struct hostinfo *hi;

	hi = gethostinfo(hostname);
	*ap = hi->addrs[0];
	freehostinfo(hi);
}

void
setsin(register struct sockaddr_in *sin, register u_int32_t addr)
{

	memset(sin, 0, sizeof(*sin));
#ifdef HAVE_SOCKADDR_SA_LEN
	sin->sin_len = sizeof(*sin);
#endif
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = addr;
}

/* String to value with optional min and max. Handles decimal and hex. */
int
str2val(register const char *str, register const char *what,
    register int mi, register int ma)
{
	register const char *cp;
	register int val;
	char *ep;

	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		cp = str + 2;
		val = (int)strtol(cp, &ep, 16);
	} else
		val = (int)strtol(str, &ep, 10);
	if (*ep != '\0') {
		Fprintf(stderr, "%s: \"%s\" bad value for %s \n",
		    prog, str, what);
		exit(1);
	}
	if (val < mi && mi >= 0) {
		if (mi == 0)
			Fprintf(stderr, "%s: %s must be >= %d\n",
			    prog, what, mi);
		else
			Fprintf(stderr, "%s: %s must be > %d\n",
			    prog, what, mi - 1);
		exit(1);
	}
	if (val > ma && ma >= 0) {
		Fprintf(stderr, "%s: %s must be <= %d\n", prog, what, ma);
		exit(1);
	}
	return (val);
}

__dead void
usage(void)
{
	extern char version[];

	Fprintf(stderr, "Version %s\n", version);
	Fprintf(stderr, "Usage: %s [-dFInrvx] [-g gateway] [-i iface] \
[-f first_ttl] [-m max_ttl]\n\t[ -p port] [-q nqueries] [-s src_addr] [-t tos] \
[-w waittime]\n\thost [packetlen]\n",
	    prog);
	exit(1);
}

int
get_src(struct sockaddr_in *dst, struct sockaddr_in *src)
{
	int s;
	struct sockaddr_in addr;
	int addrlen;

	memcpy(&addr, dst, dst->sin_len);
	addr.sin_port = port;
	addrlen = sizeof(addr);

	if ((s = socket(dst->sin_family, SOCK_DGRAM, 0)) < 0 ||
	    connect(s, (struct sockaddr *)&addr, addrlen) < 0 ||
	    getsockname(s, (struct sockaddr *)&addr, &addrlen) < 0 ||
	    addr.sin_addr.s_addr == INADDR_ANY) {
		if (s >= 0)
			close(s);
		return(0);
	}

	setsin(src, addr.sin_addr.s_addr);
	close(s);
	return(1);
}

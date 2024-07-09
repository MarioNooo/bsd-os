/*	BSDI	ping.c,v 2.17 2002/03/15 20:39:36 dab Exp	*/

#define INET6 1
#define IPSEC 1
/*
 * ping.c  --  Implement the ping(8) command.
 *
 * Copyright 1995 by Dan McDonald, Randall Atkinson, and Bao Phan
 *	All Rights Reserved.
 *	All Rights under this copyright have been assigned to NRL.
 */

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
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Muuss.
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
static char copyright[] =
"@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ping.c	8.3 (Berkeley) 4/28/95";
#endif /* not lint */

/*
 * Using the InterNet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 *
 * Status -
 *	Public Domain.  Distribution Unlimited.
 * Bugs -
 *	More statistics could always be gathered.
 *	This program has to run SUID to ROOT to access the ICMP socket.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if INET6
#include <sys/uio.h>

#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#endif /* INET6 */

#if IPSEC
#include <netinet6/ah.h>
#include <netinet6/ipsec.h>
#endif /* IPSEC */

#define	DEFDATALEN	(64 - 8)	/* default data length */
#define	MAXIPLEN	60
#if INET6
#define MAXIP6LEN	128
#endif /* INET6 */
#define	MAXICMPLEN	76
#define	MAXPACKET	(65536 - 60 - 8)/* max packet size */
#define	NROUTES		9		/* number of record route slots */

#define	A(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~B(bit)))
#define	TST(bit)	(A(bit) & B(bit))

#define	F_FLOOD		0x0001
#define	F_INTERVAL	0x0002
#define	F_NUMERIC	0x0004
#define	F_PINGFILLED	0x0008
#define	F_QUIET		0x0010
#define	F_RROUTE	0x0020
#define	F_SO_DEBUG	0x0040
#define	F_SO_DONTROUTE	0x0080
#define	F_VERBOSE	0x0100
u_int options;

/*
 * MAX_DUP_CHK is the number of bits in received table, i.e. the maximum
 * number of received sequence numbers we can keep track of.  Change 128
 * to 8192 for complete accuracy...
 */
#define	MAX_DUP_CHK	(8 * 8192)
int mx_dup_ck = MAX_DUP_CHK;
char rcvd_tbl[MAX_DUP_CHK / 8];

union {
	struct sockaddr		sa;
	struct sockaddr_in	sin;
#if INET6
	struct sockaddr_in6	sin6;
#endif /* INET6 */
} whereto;			/* who to ping */

int datalen = DEFDATALEN;
int s;				/* socket file descriptor */
u_char outpack[MAXPACKET+8];
char BSPACE = '\b';		/* characters written for flood */
char DOT = '.';
char *hostname;
int ident;			/* process id to identify our packets */
int af = 0;			/* address family */

/* counters */
long npackets;			/* max packets to transmit */
long nreceived;			/* # of packets we got back */
long nrepeats;			/* number of duplicates */
long ntransmitted;		/* sequence # for outbound packets = #sent */
int interval = 1;		/* interval between packets */

/* timing */
int timing;			/* flag to do timing */
double tmin = 999999999.0;	/* minimum round trip time */
double tmax = 0.0;		/* maximum round trip time */
double tsum = 0.0;		/* sum of all times, for doing average */

void	 fill __P((char *, char *));
u_short	 in_cksum __P((u_short *, int));
void	 onalrm __P((int));
void	 oninfo __P((int));
void	 onint __P((int));
void	 pinger __P((void));
char	*pr_addr __P((u_long));
void	 pr_icmph __P((struct icmp *));
void	 pr_iph __P((struct ip *));
#if !INET6
void	 pr_pack __P((char *, int, struct sockaddr_in *));
#else /* INET6 */
void	 pr_pack __P((char *, int, struct sockaddr *, struct msghdr *));
#endif /* INET6 */
void	 pr_retip __P((struct ip *));
void	 summary __P((void));
void	 tvsub __P((struct timeval *, struct timeval *));
void	 usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int errno, optind;
	extern char *optarg;
	struct itimerval itimer;
#if !INET6
	struct hostent *hp;
	struct sockaddr_in *to, from;
#endif /* INET6 */
	struct protoent *proto;
	struct timeval timeout;
	fd_set fdset;
	register int cc, i;
	int ch, fromlen, hold, packlen, preload;
	u_char *datap, *packet;
	char *e, *target, hnamebuf[MAXHOSTNAMELEN];
#if IPSEC
	void *request = NULL;
	int requestlen = 0;
#endif /* IPSEC */
#ifdef IP_OPTIONS
	char rspace[3 + 4 * NROUTES + 1];	/* record route space */
#endif
#if INET6
	struct addrinfo req, *ai, *ai2;
#endif /* INET6 */

	preload = 0;
	datap = &outpack[8 + sizeof(struct timeval)];
	while ((ch = getopt(argc, argv,
	    "a:46"
#if IPSEC
	    "S:"
#endif /* IPSEC */
	    "c:dfi:l:np:qRrs:v")) != EOF)
		switch(ch) {
		case 'a':
			if (strcmp(optarg, "inet") == 0)
		case '4':
				af = AF_INET;
			else if (strcmp(optarg, "inet6") == 0) {
		case '6':
#if INET6
				af = AF_INET6;
#else
				errx(1, "inet6 not supported");
#endif /* INET6 */
			} else
				errx(1, "%s: invalid family", optarg);
			break;
#if IPSEC
		case 'S':
			if (nrl_strtoreq(optarg, &request, &requestlen))
				errx(1, "nrl_strtoreq(%s) failed", optarg);
			break;
#endif /* IPSEC */
		case 'c':
			npackets = strtol(optarg, &e, 10);
			if (npackets <= 0 || *optarg == '\0' || *e != '\0')
				errx(1,
				    "illegal number of packets -- %s", optarg);
			break;
		case 'd':
			options |= F_SO_DEBUG;
			break;
		case 'f':
			if (getuid()) {
				errno = EPERM;
				err(1, NULL);
			}
			options |= F_FLOOD;
			setbuf(stdout, (char *)NULL);
			break;
		case 'i':		/* wait between sending packets */
			interval = strtol(optarg, &e, 10);
			if (interval <= 0 || *optarg == '\0' || *e != '\0')
				errx(1,
				    "illegal timing interval -- %s", optarg);
			options |= F_INTERVAL;
			break;
		case 'l':
			preload = strtol(optarg, &e, 10);
			if (preload < 0 || *optarg == '\0' || *e != '\0')
				errx(1, "illegal preload value -- %s", optarg);
			if (preload > 100 & getuid()) {
				errno = EPERM;
				err(1, NULL);
			}
			break;
		case 'n':
			options |= F_NUMERIC;
			break;
		case 'p':		/* fill buffer with user pattern */
			options |= F_PINGFILLED;
			fill((char *)datap, optarg);
				break;
		case 'q':
			options |= F_QUIET;
			break;
		case 'R':
			options |= F_RROUTE;
			break;
		case 'r':
			options |= F_SO_DONTROUTE;
			break;
		case 's':		/* size of packet to send */
			datalen = strtol(optarg, &e, 10);
			if (datalen <= 0 || *optarg == '\0' || *e != '\0')
				errx(1, "illegal datalen value -- %s", optarg);
			if (datalen > MAXPACKET)
				errx(1,
				    "datalen value too large, maximum is %d",
				    MAXPACKET);
			break;
		case 'v':
			options |= F_VERBOSE;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (options & F_FLOOD && options & F_INTERVAL)
		errx(1, "-f and -i incompatible options");

	if (argc != 1)
		usage();
	target = *argv;

#if INET6
	memset(&req, 0, sizeof(struct addrinfo));
	req.ai_family = af;

	if (i = getaddrinfo(hostname = target, NULL, &req, &ai))
		errx(1, "%s: %s", target, gai_strerror(i));

	i = 0;
	for (ai2 = ai; ai2; ai2 = ai2->ai_next) {
		if (ai2->ai_family != AF_INET && ai2->ai_family != AF_INET6)
			continue;
		i++;
		af = ai2->ai_family;

		e = (af == AF_INET6) ? "icmpv6" : "icmp";
		if ((proto = getprotobyname(e)) == NULL)
			errx(1, "unknown protocol %s", e);

		if ((s = socket(af, SOCK_RAW, proto->p_proto)) >= 0) {
			memcpy(&whereto, ai2->ai_addr, ai2->ai_addrlen);
			break;
		}
	}

	if (!ai2) {
		if (i)
			err(1, "socket");
		errx(1, "%s: no IP version 4 or 6 addresses available\n",
		    hostname);
	}

#else /* INET6 */
	if ((proto = getprotobyname("icmp")) == NULL)
		errx(1, "unknown protocol icmp");

	memset(&whereto, 0, sizeof(whereto));
	to = (struct sockaddr_in *)&whereto;
	to->sin_family = AF_INET;
	to->sin_addr.s_addr = inet_addr(target);
	if (to->sin_addr.s_addr != (u_int)-1)
		hostname = target;
	else {
		hp = gethostbyname(target);
		if (!hp)
			errx(1, "unknown host %s", target);
		to->sin_family = hp->h_addrtype;
		memmove(&to->sin_addr, hp->h_addr, hp->h_length);
		(void)strncpy(hnamebuf, hp->h_name, sizeof(hnamebuf) - 1);
		hostname = hnamebuf;
	}

	if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0)
		err(1, "socket");
#endif /* INET6 */
	(void)setuid(getuid());

	if (datalen >= sizeof(struct timeval))	/* can we time transfer */
		timing = 1;
	packlen = datalen + MAXIPLEN + MAXICMPLEN;
#if INET6
	freeaddrinfo(ai);
	if (af == AF_INET6)
		packlen += MAXIP6LEN - MAXIPLEN;
#endif /* INET6 */

	if ((packet = malloc((u_int)packlen)) == NULL)
		err(1, NULL);
	if (!(options & F_PINGFILLED))
		for (i = 8; i < datalen; ++i)
			*datap++ = i;

#if IPSEC
	if (request && setsockopt(s, SOL_SOCKET, SO_SECURITY_REQUEST, request,
	    requestlen) < 0)
		err(1, "setsockopt(SO_SECURITY_REQUEST)");
#endif /* IPSEC */

	hold = 1;
	if (options & F_SO_DEBUG)
		(void)setsockopt(s, SOL_SOCKET, SO_DEBUG, &hold, sizeof(hold));
	if (options & F_SO_DONTROUTE)
		(void)setsockopt(s,
		    SOL_SOCKET, SO_DONTROUTE, &hold, sizeof(hold));

#if INET6
	if (af == AF_INET)
#endif /* INET6 */
	/* record route option */
	if (options & F_RROUTE) {
#ifdef IP_OPTIONS
		rspace[IPOPT_OPTVAL] = IPOPT_RR;
		rspace[IPOPT_OLEN] = sizeof(rspace)-1;
		rspace[IPOPT_OFFSET] = IPOPT_MINOFF;
		if (setsockopt(s,
		    IPPROTO_IP, IP_OPTIONS, rspace, sizeof(rspace)) < 0)
			err(1, "record route");
#else
		errx(1, "record route not available in this implementation");
#endif
	}

	/*
	 * When pinging the broadcast address, you can get a lot of answers.
	 * Doing something so evil is useful if you are trying to stress the
	 * ethernet, or just want to fill the arp cache to get some stuff for
	 * /etc/ethers.
	 */
	hold = 48 * 1024;
	if (hold < packlen)
		hold = packlen;
	(void)setsockopt(s, SOL_SOCKET, SO_RCVBUF, &hold, sizeof(hold));

	/* Make sure the SO_SNDBUF is big enough for what we want to send. */
	fromlen = sizeof(hold);
	if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, &hold,
	    (size_t *)&fromlen) == 0 && hold < packlen &&
	    setsockopt(s, SOL_SOCKET, SO_SNDBUF, &packlen, sizeof(packlen)) < 0)
		err(1, "can't set socket send buffer");

#if INET6
	if (af == AF_INET6) {
		struct icmp6_filter filter;
		int i;

		ICMP6_FILTER_SETBLOCKALL(&filter);

		ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY, &filter);
		ICMP6_FILTER_SETPASS(ICMP6_DST_UNREACH, &filter);
		ICMP6_FILTER_SETPASS(ICMP6_PACKET_TOO_BIG, &filter);
		ICMP6_FILTER_SETPASS(ICMP6_TIME_EXCEEDED, &filter);
		ICMP6_FILTER_SETPASS(ICMP6_PARAM_PROB, &filter);

		if (setsockopt(s, IPPROTO_ICMPV6, ICMP6_FILTER, &filter,
		    sizeof(struct icmp6_filter)) < 0)
			err(1, "setsockopt(IPPROTO_ICMPV6, ICMP6_FILTER)");

		i = 1;
		if (setsockopt(s, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &i,
		    sizeof(int)) < 0)
			err(1, "setsockopt(IPPROTO_IPV6, IPV6_RECVHOPLIMIT)");

		i = 2;
		if (setsockopt(s, IPPROTO_IPV6, IPV6_CHECKSUM, &i,
		    sizeof(int)) < 0)
			err(1, "setsockopt(IPPROTO_IPV6, IPV6_CHECKSUM)");

	}

	{
		char buf[64];

		if (getnameinfo(&whereto.sa, whereto.sa.sa_len,
		    buf, sizeof(buf), NULL, 0, NI_NUMERICHOST))
			errx(1, "getnameinfo failed");

		printf("PING %s (%s): %d data bytes\n", hostname, buf, datalen);
	}
#else /* INET6 */
	if (to->sin_family == AF_INET)
		(void)printf("PING %s (%s): %d data bytes\n", hostname,
		    inet_ntoa(*(struct in_addr *)&to->sin_addr.s_addr),
		    datalen);
	else
		(void)printf("PING %s: %d data bytes\n", hostname, datalen);
#endif /* INET6 */

	ident = getpid() & 0xFFFF;
	while (preload--)		/* Fire off them quickies. */
		pinger();

	(void)signal(SIGINT, onint);
	(void)signal(SIGINFO, oninfo);

	if ((options & F_FLOOD) == 0) {
		(void)signal(SIGALRM, onalrm);
		itimer.it_interval.tv_sec = interval;
		itimer.it_interval.tv_usec = 0;
		itimer.it_value.tv_sec = 0;
		itimer.it_value.tv_usec = 1;
		(void)setitimer(ITIMER_REAL, &itimer, NULL);
	}

	FD_ZERO(&fdset);
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;

#if INET6
    {
	union {
		struct sockaddr_in in;
		struct sockaddr_in6 in6;
	} from;
	struct msghdr msghdr;
	struct iovec iovec;
	char control[128];
	int cc;

	memset(&msghdr, 0, sizeof(struct msghdr));
	msghdr.msg_name = (caddr_t)&from;
	msghdr.msg_iov = &iovec;
	msghdr.msg_iovlen = 1;
	if (af == AF_INET6)
		msghdr.msg_control = control;

	memset(&iovec, 0, sizeof(struct iovec));
	iovec.iov_base = (caddr_t)packet;
	iovec.iov_len = packlen;
#endif /* INET6 */

	for (;;) {

#if !INET6
		struct sockaddr_in from;
		register int cc;
		int fromlen;
#endif /* INET6 */

		if (options & F_FLOOD) {
			pinger();
			FD_SET(s, &fdset);
			if (select(s + 1, &fdset, NULL, NULL, &timeout) < 1)
				continue;
		}
#if INET6
		msghdr.msg_namelen = sizeof(from);
		if (af == AF_INET6)
			msghdr.msg_controllen = sizeof(control);

		if ((cc = recvmsg(s, &msghdr, 0)) < 0) {
			if ((errno == EAGAIN) || (errno == EINTR))
				continue;
			warn("recvmsg");
			continue;
		}
		pr_pack((char *)packet, cc, (struct sockaddr *)&from, &msghdr);
#else /* INET6 */
		fromlen = sizeof(from);
		if ((cc = recvfrom(s, packet, packlen, 0,
		    (struct sockaddr *)&from, &fromlen)) < 0) {
			if (errno == EINTR)
				continue;
			warn("recvfrom");
			continue;
		}
		pr_pack((char *)packet, cc, &from);
#endif /* INET6 */
		if (npackets && nreceived >= npackets)
			break;
	}
#if INET6
    }
#endif /* INET6 */
	summary();
	exit(nreceived == 0);
}

/*
 * onalrm --
 *	This routine transmits another ping.
 */
void
onalrm(signo)
	int signo;
{
	struct itimerval itimer;

	if (!npackets || ntransmitted < npackets) {
		pinger();
		return;
	}

	/*
	 * If we're not transmitting any more packets, change the timer
	 * to wait two round-trip times if we've received any packets or
	 * ten seconds if we haven't.
	 */
#define	MAXWAIT		10
	if (nreceived) {
		itimer.it_value.tv_sec =  2 * tmax / 1000;
		if (itimer.it_value.tv_sec == 0)
			itimer.it_value.tv_sec = 1;
	} else
		itimer.it_value.tv_sec = MAXWAIT;
	itimer.it_interval.tv_sec = 0;
	itimer.it_interval.tv_usec = 0;
	itimer.it_value.tv_usec = 0;

	(void)signal(SIGALRM, onint);
	(void)setitimer(ITIMER_REAL, &itimer, NULL);
}

/*
 * pinger --
 *	Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
void
pinger()
{
	register struct icmp *icp;
#if INET6
	register struct icmp6_hdr *icmpv6 = (struct icmp6_hdr *)outpack;
#endif /* INET6 */
	register int cc;
	int i;

	icp = (struct icmp *)outpack;

#if INET6
	/*
 	 * We could use only icp, since IPv6 icmp echo packet format
	 * is identical to IPv4, but this is cleaner at the cost of
	 * additional code
	 */
    switch(af) {
    case AF_INET:
#endif /* INET6 */
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = ntransmitted++;
	icp->icmp_id = ident;			/* ID */

	CLR(icp->icmp_seq % mx_dup_ck);
#if INET6
	break;
    case AF_INET6:
	icmpv6->icmp6_type = ICMP6_ECHO_REQUEST;
	icmpv6->icmp6_code = 0;
	icmpv6->icmp6_cksum = 0;
	icmpv6->icmp6_id = ident;
	icmpv6->icmp6_seq = ntransmitted++;
	CLR(icmpv6->icmp6_seq % mx_dup_ck);
	break;
    default:
	errx(1, "Don't know how to build an echo request for af#%d\n", af);
    }
#endif /* INET6 */

	if (timing)
		(void)gettimeofday((struct timeval *)&outpack[8], NULL);

	cc = datalen + 8;			/* skips ICMP portion */

#if INET6
	/*
	 * compute IPv4 ICMP checksum here.
	 * for IPv6, just let the kernel do the checksum calculation in
	 * ipv6_output, since the checksum now includes an ipv6 pseudo
	 * header, which requires us to know the source address of the
	 * sending interface.
	 */
	if (af == AF_INET)
	    icp->icmp_cksum = in_cksum((u_short *)icp, cc);

	i = sendto(s, (char *)outpack, cc, 0, &whereto.sa, whereto.sa.sa_len);
#else /* INET6 */
	/* compute ICMP checksum here */
	icp->icmp_cksum = in_cksum((u_short *)icp, cc);

	i = sendto(s, outpack, cc, 0, &whereto.sa, sizeof(struct sockaddr));
#endif /* INET6 */

	if (i < 0 || i != cc) {
		if (i < 0)
			warn("sendto");
		(void)printf("ping: wrote %s %d chars, ret=%d\n",
		    hostname, cc, i);
	}
	if (!(options & F_QUIET) && options & F_FLOOD)
		(void)write(STDOUT_FILENO, &DOT, 1);
}

/*
 * pr_pack --
 *	Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
void
#if INET6
pr_pack(buf, cc, from, msghdr)
#else /* INET6 */
pr_pack(buf, cc, from)
#endif /* INET6 */
	char *buf;
	int cc;
#if INET6
	struct sockaddr *from;
	struct msghdr *msghdr;
#else /* INET6 */
	struct sockaddr_in *from;
#endif /* INET6 */
{
	register struct icmp *icp;
#if INET6
	register struct icmp6_hdr *icmpv6;
#endif /* INET6 */
	register u_long l;
	register int i, j;
	register u_char *cp,*dp;
	static int old_rrlen;
	static char old_rr[MAX_IPOPTLEN];
#if !INET6
	struct ip *ip;
#endif /* INET6 */
	struct timeval tv, *tp;
	double triptime;
	int hlen, dupflag;
#if INET6
	int htype, hsize;
	char *icmpdata, name[64];
	int icmpseq;
	char ttl[4];
#endif /* INET6 */

	(void)gettimeofday(&tv, NULL);

#if INET6
	strcpy(ttl, "???");

	switch(af) {
	case AF_INET6:
		if (msghdr) {
			struct cmsghdr *cmsghdr;
			for (cmsghdr = CMSG_FIRSTHDR(msghdr); cmsghdr;
			    cmsghdr = CMSG_NXTHDR(msghdr, cmsghdr))
				if ((cmsghdr->cmsg_level == IPPROTO_IPV6) &&
				    (cmsghdr->cmsg_type == IPV6_HOPLIMIT) &&
				    (cmsghdr->cmsg_len ==
				    CMSG_LEN(sizeof(int))) &&
				    (*(int *)CMSG_DATA(cmsghdr) >= 0) &&
				    (*(int *)CMSG_DATA(cmsghdr) < 256))
					sprintf(ttl, "%d",
					    *(int *)CMSG_DATA(cmsghdr));
				    else
					printf("ttl = %d\n", *(int *)CMSG_DATA(cmsghdr));
		}
		hlen = 0;
		break;
	case AF_INET:
		snprintf(ttl, sizeof(ttl), "%d", ((struct ip *)buf)->ip_ttl);
		hlen = ((struct ip *)buf)->ip_hl << 2;
		break;
	}

	if (getnameinfo(from, from->sa_len, name, sizeof(name),
	    NULL, 0, NI_NUMERICHOST))
		errx(1, "getnameinfo failed");
#else /* INET6 */
	/* Check the IP header */
	ip = (struct ip *)buf;
	hlen = ip->ip_hl << 2;
#endif /* INET6 */

	if (cc < hlen + ICMP_MINLEN) {
		if (options & F_VERBOSE)
			warnx("packet too short (%d bytes) from %s\n", cc,
#if INET6
			  name);
#else /* INET6 */
			  inet_ntoa(*(struct in_addr *)&from->sin_addr.s_addr));
#endif /* INET6 */
		return;
	}

#if INET6
	switch(af) {
	case AF_INET6:
		icmpv6 = (struct icmp6_hdr *)buf;
		icmpdata = (char *)buf + sizeof(struct icmp6_hdr);
		icmpseq = icmpv6->icmp6_seq;
		if ((i = (icmpv6->icmp6_type == ICMP6_ECHO_REPLY)) &&
		    (icmpv6->icmp6_id != ident))
			return;
		break;
	case AF_INET:
		cc -= hlen;
		icp = (struct icmp *)(buf + hlen);
		icmpdata = (char *)&icp->icmp_data;
		icmpseq = icp->icmp_seq;
		if ((i = (icp->icmp_type == ICMP_ECHOREPLY)) &&
		    (icp->icmp_id != ident))
			return;
		break;
	}
	if (i) {
#else /* INET6 */
	/* Now the ICMP part */
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if (icp->icmp_type == ICMP_ECHOREPLY) {
		if (icp->icmp_id != ident)
			return;			/* 'Twas not our ECHO */
#endif /* INET6 */
		++nreceived;
		if (timing) {
#if INET6
			tp = (struct timeval *)icmpdata;
#else /* INET6 */
#ifndef icmp_data
			tp = (struct timeval *)&icp->icmp_ip;
#else
			tp = (struct timeval *)icp->icmp_data;
#endif
#endif /* INET6 */
			tvsub(&tv, tp);
			triptime = ((double)tv.tv_sec) * 1000.0 +
			    ((double)tv.tv_usec) / 1000.0;
			tsum += triptime;
			if (triptime < tmin)
				tmin = triptime;
			if (triptime > tmax)
				tmax = triptime;
		}

#if INET6
		if (TST(icmpseq % mx_dup_ck)) {
#else /* INET6 */
		if (TST(icp->icmp_seq % mx_dup_ck)) {
#endif /* INET6 */
			++nrepeats;
			--nreceived;
			dupflag = 1;
		} else {
#if INET6
			SET(icmpseq % mx_dup_ck);
#else /* INET6 */
			SET(icp->icmp_seq % mx_dup_ck);
#endif /* INET6 */
			dupflag = 0;
		}

		if (options & F_QUIET)
			return;

		if (options & F_FLOOD)
			(void)write(STDOUT_FILENO, &BSPACE, 1);
		else {
			(void)printf("%d bytes from %s%c icmp_seq=%u", cc,
#if INET6
			   name, (af == AF_INET6) ? ',' : ':', icmpseq);
			(void)printf(" %s=%s",
			   (af == AF_INET6) ? "hlim" : "ttl", ttl);
#else /* INET6 */
			   inet_ntoa(*(struct in_addr *)&from->sin_addr.s_addr),
			   ',', icp->icmp_seq);
			(void)printf(" ttl=%d", ip->ip_ttl);
#endif /* INET6 */
			if (timing)
				(void)printf(" time=%g ms", triptime);
			if (dupflag)
				(void)printf(" (DUP!)");
			/* check the data */
#if INET6
			cp = (u_char*)(&icmpdata[8]);
#else /* INET6 */
			cp = (u_char*)&icp->icmp_data[8];
#endif /* INET6 */
			dp = &outpack[8 + sizeof(struct timeval)];
			for (i = 8; i < datalen; ++i, ++cp, ++dp) {
				if (*cp != *dp) {
	(void)printf("\nwrong data byte #%d should be 0x%x but was 0x%x",
	    i, *dp, *cp);
#if INET6
					cp = (u_char*)&icmpdata[8];
#else /* INET6 */
					cp = (u_char*)&icp->icmp_data[8];
#endif /* INET6 */
					for (i = 8; i < datalen; ++i, ++cp) {
						if ((i % 32) == 8)
							(void)printf("\n\t");
						(void)printf("%x ", *cp);
					}
					break;
				}
			}
		}
	} else {
		/* We've got something other than an ECHOREPLY */
		if (!(options & F_VERBOSE))
			return;
		(void)printf("%d bytes from %s: ", cc,
#if INET6
		    name);
		if (af == AF_INET)
			pr_icmph(icp);
		else
			pr_icmpv6h(buf);
#else /* INET6 */
		    pr_addr(from->sin_addr.s_addr));
		pr_icmph(icp);
#endif /* INET6 */
	}

	/* Display any IP options */
#if INET6
	/* We need to do the same for IPv6 */
	if (af == AF_INET) {
#endif /* INET6 */
	cp = (u_char *)buf + sizeof(struct ip);

	for (; hlen > (int)sizeof(struct ip); --hlen, ++cp)
		switch (*cp) {
		case IPOPT_EOL:
			hlen = 0;
			break;
		case IPOPT_LSRR:
			(void)printf("\nLSRR: ");
			hlen -= 2;
			j = *++cp;
			++cp;
			if (j > IPOPT_MINOFF)
				for (;;) {
					l = *++cp;
					l = (l<<8) + *++cp;
					l = (l<<8) + *++cp;
					l = (l<<8) + *++cp;
					if (l == 0)
						(void)printf("\t0.0.0.0");
				else
					(void)printf("\t%s", pr_addr(ntohl(l)));
				hlen -= 4;
				j -= 4;
				if (j <= IPOPT_MINOFF)
					break;
				(void)putchar('\n');
			}
			break;
		case IPOPT_RR:
			j = *++cp;		/* get length */
			i = *++cp;		/* and pointer */
			hlen -= 2;
			if (i > j)
				i = j;
			i -= IPOPT_MINOFF;
			if (i <= 0)
				continue;
			if (i == old_rrlen
			    && cp == (u_char *)buf + sizeof(struct ip) + 2
			    && !memcmp(cp, old_rr, i)
			    && !(options & F_FLOOD)) {
				(void)printf("\t(same route)");
				i = ((i + 3) / 4) * 4;
				hlen -= i;
				cp += i;
				break;
			}
			old_rrlen = i;
			memmove(old_rr, cp, i);
			(void)printf("\nRR: ");
			for (;;) {
				l = *++cp;
				l = (l<<8) + *++cp;
				l = (l<<8) + *++cp;
				l = (l<<8) + *++cp;
				if (l == 0)
					(void)printf("\t0.0.0.0");
				else
					(void)printf("\t%s", pr_addr(ntohl(l)));
				hlen -= 4;
				i -= 4;
				if (i <= 0)
					break;
				(void)putchar('\n');
			}
			break;
		case IPOPT_NOP:
			(void)printf("\nNOP");
			break;
		default:
			(void)printf("\nunknown option %x", *cp);
			break;
		}
#if INET6
	}
#endif /* INET6 */
	if (!(options & F_FLOOD)) {
		(void)putchar('\n');
		(void)fflush(stdout);
	}
}

/*
 * in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 */
u_short
in_cksum(addr, len)
	u_short *addr;
	int len;
{
	u_short *w = addr;
	u_int sum = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	for (; len > 1; len -= 2)
		sum += *w++;

	/* mop up an odd byte, if necessary */
	if (len == 1)
		sum += ntohs(*(u_char *)w << 8);

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += sum >> 16;
	return (0xffff & ~sum);
}

/*
 * tvsub --
 *	Subtract 2 timeval structs:  out = out - in.  Out is assumed to
 * be >= in.
 */
void
tvsub(out, in)
	register struct timeval *out, *in;
{
	if ((out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

/*
 * oninfo --
 *	SIGINFO handler.
 */
void
oninfo(signo)
	int signo;
{
	summary();
}

/*
 * onint --
 *	SIGINT and final SIGALRM handler.
 */
void
onint(signo)
	int signo;
{
	summary();

	if (signo == SIGINT) {
		(void)signal(SIGINT, SIG_DFL);
		(void)kill(getpid(), SIGINT);
		/* NOTREACHED */
	}
	exit(1);
}

/*
 * summary --
 *	Print out statistics.
 */
void
summary()
{
	register int i;

	(void)printf("\n--- %s ping statistics ---\n", hostname);
	(void)printf("%ld packets transmitted, ", ntransmitted);
	(void)printf("%ld packets received, ", nreceived);
	if (nrepeats)
		(void)printf("+%ld duplicates, ", nrepeats);
	if (ntransmitted)
		if (nreceived > ntransmitted)
			(void)printf("-- somebody's printing up packets!");
		else
			(void)printf("%d%% packet loss",
			    (int) (((ntransmitted - nreceived) * 100) /
			    ntransmitted));
	(void)putchar('\n');
	if (nreceived && timing) {
		/* Only display average to microseconds */
		i = 1000.0 * tsum / (nreceived + nrepeats);
		(void)printf("round-trip min/avg/max = %g/%g/%g ms\n",
		    tmin, ((double)i) / 1000.0, tmax);
	}
	(void)fflush(stdout);
}

#ifdef notdef
static char *ttab[] = {
	"Echo Reply",		/* ip + seq + udata */
	"Dest Unreachable",	/* net, host, proto, port, frag, sr + IP */
	"Source Quench",	/* IP */
	"Redirect",		/* redirect type, gateway, + IP  */
	"Echo",
	"Time Exceeded",	/* transit, frag reassem + IP */
	"Parameter Problem",	/* pointer + IP */
	"Timestamp",		/* id + seq + three timestamps */
	"Timestamp Reply",	/* " */
	"Info Request",		/* id + sq */
	"Info Reply"		/* " */
};
#endif

/*
 * pr_icmph --
 *	Print a descriptive string about an ICMP header.
 */
void
pr_icmph(icp)
	struct icmp *icp;
{
	switch(icp->icmp_type) {
	case ICMP_ECHOREPLY:
		(void)printf("Echo Reply\n");
		/* XXX ID + Seq + Data */
		break;
	case ICMP_UNREACH:
		switch(icp->icmp_code) {
		case ICMP_UNREACH_NET:
			(void)printf("Destination Net Unreachable\n");
			break;
		case ICMP_UNREACH_HOST:
			(void)printf("Destination Host Unreachable\n");
			break;
		case ICMP_UNREACH_PROTOCOL:
			(void)printf("Destination Protocol Unreachable\n");
			break;
		case ICMP_UNREACH_PORT:
			(void)printf("Destination Port Unreachable\n");
			break;
		case ICMP_UNREACH_NEEDFRAG:
			(void)printf("frag needed and DF set\n");
			break;
		case ICMP_UNREACH_SRCFAIL:
			(void)printf("Source Route Failed\n");
			break;
		default:
			(void)printf("Dest Unreachable, Bad Code: %d\n",
			    icp->icmp_code);
			break;
		}
		/* Print returned IP header information */
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_SOURCEQUENCH:
		(void)printf("Source Quench\n");
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_REDIRECT:
		switch(icp->icmp_code) {
		case ICMP_REDIRECT_NET:
			(void)printf("Redirect Network");
			break;
		case ICMP_REDIRECT_HOST:
			(void)printf("Redirect Host");
			break;
		case ICMP_REDIRECT_TOSNET:
			(void)printf("Redirect Type of Service and Network");
			break;
		case ICMP_REDIRECT_TOSHOST:
			(void)printf("Redirect Type of Service and Host");
			break;
		default:
			(void)printf("Redirect, Bad Code: %d", icp->icmp_code);
			break;
		}
		(void)printf("(New addr: 0x%08lx)\n", icp->icmp_gwaddr.s_addr);
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_ECHO:
		(void)printf("Echo Request\n");
		/* XXX ID + Seq + Data */
		break;
	case ICMP_TIMXCEED:
		switch(icp->icmp_code) {
		case ICMP_TIMXCEED_INTRANS:
			(void)printf("Time to live exceeded\n");
			break;
		case ICMP_TIMXCEED_REASS:
			(void)printf("Frag reassembly time exceeded\n");
			break;
		default:
			(void)printf("Time exceeded, Bad Code: %d\n",
			    icp->icmp_code);
			break;
		}
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_PARAMPROB:
		(void)printf("Parameter problem: pointer = 0x%02x\n",
		    icp->icmp_hun.ih_pptr);
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_TSTAMP:
		(void)printf("Timestamp\n");
		/* XXX ID + Seq + 3 timestamps */
		break;
	case ICMP_TSTAMPREPLY:
		(void)printf("Timestamp Reply\n");
		/* XXX ID + Seq + 3 timestamps */
		break;
	case ICMP_IREQ:
		(void)printf("Information Request\n");
		/* XXX ID + Seq */
		break;
	case ICMP_IREQREPLY:
		(void)printf("Information Reply\n");
		/* XXX ID + Seq */
		break;
#ifdef ICMP_MASKREQ
	case ICMP_MASKREQ:
		(void)printf("Address Mask Request\n");
		break;
#endif
#ifdef ICMP_MASKREPLY
	case ICMP_MASKREPLY:
		(void)printf("Address Mask Reply\n");
		break;
#endif
	default:
		(void)printf("Bad ICMP type: %d\n", icp->icmp_type);
	}
}

#if INET6
/*
 * pr_icmpv6h --
 *	Print a descriptive string about an ICMPv6 header.
 */
pr_icmpv6h(char *cp)
{
	struct icmp6_hdr *icmpv6 = (struct icmp6_hdr *)cp;

	switch(icmpv6->icmp6_type) {
	case ICMP6_ECHO_REPLY:
		printf("Echo Reply\n");
		break;
	case ICMP6_DST_UNREACH:
		switch(icmpv6->icmp6_code) {
		case ICMP6_DST_UNREACH_NOROUTE:
			printf("Destination Route Not Available\n");
			break;
		case ICMP6_DST_UNREACH_ADMIN:
			printf("Destination Administratively Prohibited\n");
			break;
		case ICMP6_DST_UNREACH_NOTNEIGHBOR:
			printf("Destination Neighbor Unreachable\n");
			break;
		case ICMP6_DST_UNREACH_ADDR:
			printf("Destination Address Unreachable\n");
			break;
		case ICMP6_DST_UNREACH_NOPORT:
			printf("Destination Port Unreachable\n");
			break;
		default:
			printf("Dest Unreachable, Bad Code: %d\n", icmpv6->icmp6_code);
			break;
		}
		pr_retip((struct ip *)(cp + sizeof(struct icmp6_hdr)));
		break;
	case ICMP6_PACKET_TOO_BIG:
		printf("Packet too big\n");
		pr_retip((struct ip *)(cp + sizeof(struct icmp6_hdr)));
		break;
	case ICMP6_TIME_EXCEEDED:
		switch(icmpv6->icmp6_code) {
		case ICMP6_TIME_EXCEED_TRANSIT:
			printf("Hop limit exceeded\n");
			break;
		case ICMP6_TIME_EXCEED_REASSEMBLY:
			printf("Frag reassembly time exceeded\n");
			break;
		default:
			printf("Time exceeded, Bad Code: %d\n", icmpv6->icmp6_code);
			break;
		}
		pr_retip((struct ip *)(cp + sizeof(struct icmp6_hdr)));
		break;
	case ICMP6_PARAM_PROB:
		switch(icmpv6->icmp6_code) {
		case ICMP6_PARAMPROB_HEADER:
			printf("Parameter Problem\n");
			break;
		case ICMP6_PARAMPROB_NEXTHEADER:
			printf("Bad Next Header\n");
			break;
		case ICMP6_PARAMPROB_OPTION:
			printf("Unrecognized IPv6 Option\n");
			break;
		}
		pr_retip((struct ip *)(cp + sizeof(struct icmp6_hdr)));
		break;
	default:
		printf("Bad ICMPv6 type: %d\n", icmpv6->icmp6_type);
	}
}
#endif /* INET6 */

/*
 * pr_iph --
 *	Print an IP header with options.
 */
void
pr_iph(ip)
	struct ip *ip;
{
	int hlen;
	u_char *cp;

#if INET6
    if (af == AF_INET) {
#endif /* INET6 */
	hlen = ip->ip_hl << 2;
	cp = (u_char *)ip + 20;		/* point to options */

	(void)printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst Data\n");
	(void)printf(" %1x  %1x  %02x %04x %04x",
	    ip->ip_v, ip->ip_hl, ip->ip_tos, ip->ip_len, ip->ip_id);
	(void)printf("   %1x %04x", ((ip->ip_off) & 0xe000) >> 13,
	    (ip->ip_off) & 0x1fff);
	(void)printf("  %02x  %02x %04x", ip->ip_ttl, ip->ip_p, ip->ip_sum);
	(void)printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->ip_src.s_addr));
	(void)printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->ip_dst.s_addr));
	/* dump and option bytes */
	while (hlen-- > 20)
		(void)printf("%02x", *cp++);
	(void)putchar('\n');
#if INET6
    } else if (af == AF_INET6) {
	/* IPv6 header (no options for now) */
	struct ip6_hdr *ipv6 = (struct ip6_hdr *)ip;
	struct sockaddr_in6 sin6;
	char hbuf1[64], hbuf2[64];

	memset(&sin6, 0, sizeof(struct sockaddr_in6));
#if SALEN
	sin6.sin6_len = sizeof(struct sockaddr_in6);
#endif /* SALEN */
	sin6.sin6_family = AF_INET6;

	sin6.sin6_addr = ipv6->ip6_src;
	if (getnameinfo((struct sockaddr *)&sin6,
	    sizeof(struct sockaddr_in6), hbuf1, sizeof(hbuf1), NULL, 0,
	    (options & F_NUMERIC) ? NI_NUMERICHOST : 0))
		errx(1, "getnameinfo failed");

	sin6.sin6_addr = ipv6->ip6_dst;
	if (getnameinfo((struct sockaddr *)&sin6,
	    sizeof(struct sockaddr_in6), hbuf2, sizeof(hbuf2), NULL, 0,
	    (options & F_NUMERIC) ? NI_NUMERICHOST : 0))
		errx(1, "getnameinfo failed (%d)");

	(void)printf("V FlowID  Len  Nxt Hop Src -> Dst\n");
	(void)printf("%1x %07x %04x %2x  %2x  %s -> %s\n",
	    ipv6->ip6_vfc>>4,
	    ntohl(ipv6->ip6_flow & IPV6_FLOWINFO_MASK),
	    ntohs(ipv6->ip6_plen),
	    ipv6->ip6_nxt, ipv6->ip6_hlim, hbuf1, hbuf2);
    }
#endif /* INET6 */
}

/*
 * pr_addr --
 *	Return an ascii host address as a dotted quad and optionally with
 * a hostname.
 */
char *
pr_addr(l)
	u_long l;
{
	struct hostent *hp;
	static char buf[80];

	if ((options & F_NUMERIC) ||
	    !(hp = gethostbyaddr((char *)&l, 4, AF_INET)))
		(void)snprintf(buf, sizeof(buf),
		    "%s", inet_ntoa(*(struct in_addr *)&l));
	else
		(void)snprintf(buf, sizeof(buf),
		    "%s (%s)", hp->h_name, inet_ntoa(*(struct in_addr *)&l));
	return (buf);
}

/*
 * pr_retip --
 *	Dump some info on a returned (via ICMP) IP packet.
 */
void
pr_retip(ip)
	struct ip *ip;
{
	int hlen;
	u_char *cp;
	int proto;

	pr_iph(ip);

#if INET6
	if (af == AF_INET) {
#endif /* INET6 */
		proto = ip->ip_p;
		hlen = ip->ip_hl << 2;
		cp = (u_char *)ip + hlen;
#if INET6
	} else {
		int size;

		cp = (u_char *)ip;
		/* Parse packet for transport layer data */
		proto = ((struct ip6_hdr *)ip)->ip6_nxt;
		size = sizeof(struct ip6_hdr);

		do {
			cp += size;
			switch(proto) {
#ifdef IPPROTO_HOPOPTS
			case IPPROTO_HOPOPTS:
				/* punt for now*/
				size =
				    ((struct ip6_hbh *)cp)->ip6h_len * 8 + 8;
				proto = ((struct ip6_hbh *)cp)->ip6h_nxt;
			break;
#endif /* IPPROTO_HOPOPTS */
#if IPSEC
			case IPPROTO_AH:
				size =
				    ((struct ah *)cp)->ah_len * 8 + 8;
				proto = ((struct ah *)cp)->ah_nxt;
				break;
#endif /* IPSEC */
#ifdef IPPROTO_ROUTING
#define I6SR(cp) ((struct ip6_rthdr0 *)(cp))
			case IPPROTO_ROUTING:
				if (I6SR(cp)->ip6r0_type != 0) {
					warnx("ipv6 routing type unknown\n");
					return;
				}
				size = I6SR(cp)->ip6r0_len * 8 + 8;
				proto = I6SR(cp)->ip6r0_nxt;
				break;
#endif /* IPPROTO_ROUTING */
#ifdef IPPROTO_FRAGMENT
			case IPPROTO_FRAGMENT:
				size = 8;
				proto = ((struct ip6_frag *)cp)->ip6f_nxt;
				break;
#endif /* IPPROTO_FRAGMENT */
			default:
				size = -1;
				break;
			}
		} while (size > 0);
	}

#endif /* INET6 */
	if (proto == 6)
		(void)printf("TCP: from port %u, to port %u (decimal)\n",
		    (*cp * 256 + *(cp + 1)), (*(cp + 2) * 256 + *(cp + 3)));
	else if (proto == 17)
		(void)printf("UDP: from port %u, to port %u (decimal)\n",
			(*cp * 256 + *(cp + 1)), (*(cp + 2) * 256 + *(cp + 3)));
}

void
fill(bp, patp)
	char *bp, *patp;
{
	register int ii, jj, kk;
	int pat[16];
	char *cp;

	for (cp = patp; *cp; cp++)
		if (!isxdigit(*cp))
			errx(1, "patterns must be specified as hex digits");
	ii = sscanf(patp,
	    "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &pat[0], &pat[1], &pat[2], &pat[3], &pat[4], &pat[5], &pat[6],
	    &pat[7], &pat[8], &pat[9], &pat[10], &pat[11], &pat[12],
	    &pat[13], &pat[14], &pat[15]);

	if (ii > 0)
		for (kk = 0;
		    kk <= MAXPACKET - (8 + sizeof(struct timeval) + ii);
		    kk += ii)
			for (jj = 0; jj < ii; ++jj)
				bp[jj + kk] = pat[jj];
	if (!(options & F_QUIET)) {
		(void)printf("PATTERN: 0x");
		for (jj = 0; jj < ii; ++jj)
			(void)printf("%02x", bp[jj] & 0xFF);
		(void)printf("\n");
	}
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: ping [-dfnqRrv46] "
	    "[-a family] "
#if IPSEC
	    "[-S security request] "
#endif /* IPSEC */
	    "[-c count] [-i wait]\n"
	    "            [-l preload] [-p pattern] [-s packetsize] host\n");
	exit(1);
}

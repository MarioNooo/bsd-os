/*
 * route.c  --  Implement the route(8) command.
 *
 * Copyright 1995 by Dan McDonald, Randall Atkinson, and Bao Phan
 *      All Rights Reserved.  
 *      All Rights under this copyright have been assigned to NRL.
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
 *
 * Copyright (c) 1983, 1989, 1991, 1993
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
static char copyright[] =
"@(#) Copyright (c) 1983, 1989, 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)route.c	8.6 (Berkeley) 4/28/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/route.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#ifdef XNS
#include <netns/ns.h>
#endif
#ifdef ISO
#include <netiso/iso.h>
#endif
#ifdef X25
#include <netccitt/x25.h>
#endif
#include <arpa/inet.h>
#include <netdb.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <irs.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct keytab {
	char	*kt_cp;
	int	kt_i;
} keywords[] = {
#include "keywords.h"
	{ NULL, 0 }
};

typedef union sockunion {
	struct	sockaddr sa;
	struct	sockaddr_in sin;
#ifdef XNS
	struct	sockaddr_ns sns;
#endif
#ifdef ISO
	struct	sockaddr_iso siso;
#endif
	struct	sockaddr_dl sdl;
#ifdef X25
	struct	sockaddr_x25 sx25;
#endif
	struct  sockaddr_in6 sin6;
} sockunion;

typedef struct sockinfo {
	int	rti_addrs;
	sockunion *rti_info[RTAX_MAX];
#define	rti_dst		rti_info[RTAX_DST]
#define	rti_gateway	rti_info[RTAX_GATEWAY]
#define	rti_netmask	rti_info[RTAX_NETMASK]
#define	rti_genmask	rti_info[RTAX_GENMASK]
#define	rti_ifa		rti_info[RTAX_IFA]
#define	rti_ifp		rti_info[RTAX_IFP]
#define	rti_author	rti_info[RTAX_AUTHOR]
#define	rti_brd		rti_info[RTAX_BRD]
} sockinfo;

static int	af = AF_UNSPEC;
static int	aflen = sizeof (struct sockaddr_in);
static int	debugonly;
static int	forcehost;
static int	forcenet;
static int	locking;
static int	lockrest;
static int	netmask_specified;	/* Netmask explicitly specified */
static int	nflag;
static int	qflag;
static struct rt_metrics rt_metrics;
static int	rtm_addrs;
static u_long	rtm_inits;
static int	s;
static sockunion so_dst;
static sockunion so_gate;
static sockunion so_mask;
static sockunion so_genmask;
static sockunion so_ifa;
static sockunion so_ifp;
static uid_t	uid;
static int	verbose;

static struct in6_addr in6addr_tdmask = { { {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0, 0, 0, 0
} } };
static short	ns_nullh[] = { 0, 0, 0 };
static short	ns_bh[] = { -1, -1, -1 };

static char	*af_to_name(u_short af);
static char	*bprintf(int, u_char *);
#ifdef X25
extern int	ccitt_addr(char *, struct sockaddr_x25 *);
#endif
static void	flushroutes(int, char *argc[]);
static void	getaddr(int, char *, struct addrinfo **);
static int	keyword(char *);
static void	inet_makenetandmask(u_int32_t, struct sockaddr_in *);
static void	interfaces(void);
static void	mask_addr(void);
static void	monitor(void);
static int	newroute(int, int, char *argc[]);
#ifdef XNS
static char	*ns_print(struct sockaddr_ns *);
#endif
static void	pmsg_addrs(void *, int);
static void	pmsg_common(struct rt_msghdr *);
static void	print_getmsg(struct rt_msghdr *, int);
static void	print_response(struct rt_msghdr *, int, char *);
static void	print_rtmsg(struct rt_msghdr *, int);
static char	*routename(sockunion *, sockunion *, int, int *);
static int	rtmsg(struct rt_msghdr *, int, int);
static void	set_metric(char *, int);
static void	sockaddr(char *, sockunion *);
static __dead void usage(char *);
#ifdef X25
static void	x25_makemask(void);
#endif
static void	xaddrs(void *, int, sockinfo *);
static void	xaddrs_free(sockinfo *);
static int	prefixlen();

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa.sa_len))

int
main(int argc, char *argv[])
{
	int ch;
	int tflag;

	tflag = 0;

	if (argc < 2)
		usage(NULL);

	while ((ch = getopt(argc, argv, "nqdtv")) != EOF)
		switch(ch) {
		case 'n':
			nflag = NI_NUMERICHOST;
			break;
		case 'q':
			qflag = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 't':
			tflag = 1;
			break;
		case 'd':
			debugonly = 1;
			break;
		case '?':
		default:
			usage(NULL);
		}
	argc -= optind;
	argv += optind;

	uid = getuid();
	if (tflag)
		s = open("/dev/null", O_WRONLY, 0);
	else
		s = socket(PF_ROUTE, SOCK_RAW, 0);
	if (s < 0)
		err(2, "socket");
	if (*argv != NULL)
		switch (keyword(*argv)) {
		case K_GET:
			return (newroute(RTM_GET, argc, argv));

		case K_CHANGE:
			return (newroute(RTM_CHANGE, argc, argv));

		case K_ADD:
			return (newroute(RTM_ADD, argc, argv));

		case K_DELETE:
			return (newroute(RTM_DELETE, argc, argv));

		case K_MONITOR:
			monitor();
			/* NOTREACHED */

		case K_FLUSH:
			flushroutes(argc, argv);
			exit(0);
			/* NOTREACHED */
		}
	usage(*argv);

	/* NOTREACHED */
	return (0);
}

static __dead void
usage(char *cp)
{
	if (cp != NULL)
		warnx("parsing error at: %s", cp);
	(void)fprintf(stderr,
	    "usage: route [ -nqv ] cmd [[ -<qualifers> ] args ]\n");
	exit(2);
	/* NOTREACHED */
}

/*
 * Purge all entries in the routing tables not
 * associated with network interfaces.
 */
static void
flushroutes(int argc, char *argv[])
{
	struct rt_msghdr *rtm;
	size_t needed;
	int mib[6];
	int rlen;
	int seqno;
	char *buf;
	char *lim;

	if (uid != 0) {
		errno = EACCES;
		err(2, "must be root to alter routing table");
	}
	/* Don't want to read back our messages */
	(void)shutdown(s, 0);
	if (argc > 1) {
		argv++;
		if (argc == 2 && **argv == '-')
			switch (keyword(*argv + 1)) {
			case K_INET:
				af = AF_INET;
				break;
			case K_INET6:
				af = AF_INET6;
				break;
#ifdef XNS
			case K_XNS:
				af = AF_NS;
				break;
#endif
			case K_LINK:
				af = AF_LINK;
				break;
#ifdef ISO
			case K_ISO:
			case K_OSI:
				af = AF_ISO;
				break;
#endif
#ifdef X25
			case K_X25:
				af = AF_CCITT;
#endif
			default:
				goto bad;
			}
		else
bad:
			usage(*argv);
	}

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;		/* protocol */
	mib[3] = 0;		/* wildcard address family */
	mib[4] = NET_RT_DUMP;
	mib[5] = 0;		/* no flags */
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		err(2, "route-sysctl-estimate");
	if ((buf = malloc(needed)) == NULL)
		err(2, "malloc");
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		err(2, "actual retrieval of routing table");
	lim = buf + needed;
	if (verbose)
		(void)printf("Examining routing table from sysctl\n");
	seqno = 0;		/* ??? */
	for (rtm = (struct rt_msghdr *)buf; (char *)rtm < lim; 
	     rtm = (struct rt_msghdr *)((char *)rtm + rtm->rtm_msglen)) {
		if (verbose)
			print_rtmsg(rtm, rtm->rtm_msglen);
		if ((rtm->rtm_flags & RTF_GATEWAY) == 0)
			continue;
		if (af != AF_UNSPEC &&
		    ((struct sockaddr *)(rtm + 1))->sa_family != af)
			continue;
		rtm->rtm_type = RTM_DELETE;
		rtm->rtm_seq = seqno++;
		if (debugonly)
			rlen = 0;
		else
			rlen = write(s, rtm, rtm->rtm_msglen);
		if (rlen == -1) {
			rtm->rtm_errno = errno;
			if (rtm->rtm_errno == ESRCH &&
			    (rtm->rtm_flags & RTF_CLONED)) {
				if (verbose)
					print_rtmsg(rtm, rlen);
				continue;
			}
			errno = rtm->rtm_errno;
			warn("writing to routing socket: %s");
			break;
		} else if (rlen == 0) {
			warnx("end-of-file on routing socket");
			break;
		} else if (rlen != rtm->rtm_msglen) {
			warnx("partial write on routing socket");
			break;
		}
		if (qflag)
			continue;
		if (verbose)
			print_rtmsg(rtm, rlen);
		else
			print_response(rtm, rlen, NULL);
	}
}

static char *
af_to_name(u_short af)
{
	static char buf[12];

	switch(af) {
	case AF_LINK:		
		return("link");

	case AF_INET:
		return("inet");

	case AF_INET6:
		return("inet6");

	case AF_ISO: 
		return("iso");

	case AF_NS:
		return("xns");

	default:	
		break;
	}
	(void)snprintf(buf, sizeof(buf), "af#%d", af);
	return(buf);
}

static char *
routename(sockunion *su, sockunion *mask, int flags, int *is_default)
{
	static char line[NI_MAXHOST];
	int len;
	u_short *s;
	u_short *slim;
	char *lp;

	if (is_default != NULL)
		*is_default = 0;

	if (su->sa.sa_len == 0) {
	Default:
		if (is_default != NULL)
			*is_default = 1;
		return("default");
	}

	lp = line;
	len = su->sa.sa_len;

	switch (su->sa.sa_family) {
	case AF_INET:
		if (su->sin.sin_addr.s_addr == INADDR_ANY)
			goto Default;
		if (mask == NULL) {
			len = sizeof(su->sin);
			break;
		}
		/* 
		 * The following grot should go away when there is a
		 * better interface into IRS for printing a network.
		 */
	{
		static struct irs_acc *irs_gen;
		static struct irs_nw *irs_nw;
		struct nwent *np;
		u_int32_t hmask;
		u_int32_t hnet;
		u_int32_t i;
		u_int32_t omask;
		int subnetshift;
		int maskbits;
		u_char addr[4];

		i = ntohl(su->sin.sin_addr.s_addr);
		hmask = ntohl(mask->sin.sin_addr.s_addr);
		*line = 0;
		np = NULL;
		omask = hmask;
		subnetshift = 0;
		if (irs_gen == NULL)
			irs_gen = irs_gen_acc("", NULL);
		if (irs_gen != NULL && irs_nw == NULL)
			irs_nw = (*irs_gen->nw_map)(irs_gen);
		if (!nflag && i && irs_nw != NULL) {
			if (hmask == 0) {
				if (IN_CLASSA(i)) {
					hmask = IN_CLASSA_NET;
					subnetshift = 8;
				} else if (IN_CLASSB(i)) {
					hmask = IN_CLASSB_NET;
					subnetshift = 8;
				} else {
					hmask = IN_CLASSC_NET;
					subnetshift = 4;
				}
				/*
				 * If there are more bits than the standard mask
				 * would suggest, subnets must be in use.
				 * Guess at the subnet mask, assuming reasonable
				 * width subnet fields.
				 */
				while (i &~ hmask)
					hmask = (int32_t)hmask >> subnetshift;
			}
			hnet = i & hmask;
			addr[0] = (0xFF000000 & hnet) >> 24;
			addr[1] = (0x00FF0000 & hnet) >> 16;
			addr[2] = (0x0000FF00 & hnet) >> 8;
			addr[3] = (0x000000FF & hnet);
			for (maskbits = 32; !(hmask & 1); hmask >>= 1)
				maskbits--;
			np = (*irs_nw->byaddr)(irs_nw, addr, maskbits, AF_INET);
		}
#define C(x)	(u_long)((x) & 0xff)
		if (np != NULL)
			strncat(line, np->n_name, sizeof(line) - 1);
		else if ((i & 0x00ffffff) == 0 && (omask & 0x00ffffff) == 0)
			(void)sprintf(line, "%lu", C(i >> 24));
		else if ((i & 0x0000ffff) == 0 && (omask & 0x0000ffff) == 0)
			(void)sprintf(line, "%lu.%lu", C(i >> 24) , C(i >> 16));
		else if ((i & 0x000000ff) == 0 && (omask & 0x000000ff) == 0)
			(void)sprintf(line, "%lu.%lu.%lu",
			    C(i >> 24), C(i >> 16), C(i >> 8));
		else
			(void)sprintf(line, "%lu.%lu.%lu.%lu",
			    C(i >> 24), C(i >> 16), C(i >> 8), C(i));
#undef	C
		return (line);
	}

	case AF_INET6:
		if (IN6_IS_ADDR_UNSPECIFIED(&su->sin6.sin6_addr)) {
			if (mask != NULL && 
			    memcmp(&in6addr_tdmask, &mask->sin6.sin6_addr,
			    sizeof(mask->sin6.sin6_addr)) == 0) {
				if (is_default != NULL)
					*is_default = 1;
				return("transdefault");
			}
			if (mask != NULL && (mask->sin6.sin6_len == 0
			    || memcmp(&in6addr_any, &mask->sin6.sin6_addr,
			    sizeof(mask->sin6.sin6_addr)) == 0))
				goto Default;
		}
		if ((IN6_IS_ADDR_LINKLOCAL(&su->sin6.sin6_addr) ||
		    IN6_IS_ADDR_MC_LINKLOCAL(&su->sin6.sin6_addr)) &&
		    su->sin6.sin6_scope_id == 0) {
			su->sin6.sin6_scope_id =
				ntohs(*(u_int16_t *)&su->sin6.sin6_addr.s6_addr[2]);
			su->sin6.sin6_addr.s6_addr[2] = 0;
			su->sin6.sin6_addr.s6_addr[3] = 0;
		}
		len = sizeof(su->sin6);
		break;

#ifdef XNS
	case AF_NS:
		return (ns_print(&su->sns));
#endif

	case AF_LINK:
		if (su->sdl.sdl_nlen != 0) {
			(void)snprintf(line, sizeof(line), "%.*s",
			    su->sdl.sdl_nlen, su->sdl.sdl_data);
			return (line);
		}
		break;

	case AF_ISO:
		lp += snprintf(line, sizeof(line), "iso ");
		break;

	default:
		goto numeric;
	}
#ifdef NI_WITHSCOPEID
	flags |= NI_WITHSCOPEID;
#endif 
	if (getnameinfo(&su->sa, len, lp, line + sizeof(line) - lp,
	    NULL, 0, nflag | NI_NOFQDN | flags)
	    == 0)
		return (line);

numeric:
	s = (u_short *)su;
	slim = s + ((su->sa.sa_len + 1) >> 1);
	lp = line + snprintf(line, sizeof(line), "%s",
	    af_to_name(su->sa.sa_family));

	/* start with sa->sa_data */
	while (++s < slim)
		lp += snprintf(lp, sizeof(line) - (lp - line), " %hx", *s);

	return (line);
}

static void
set_metric(char *value, int key)
{
	int flag;
	u_int32_t noval;
	u_int32_t *valp;

	flag = 0;
	valp = &noval;
	switch (key) {
#define caseof(x, y, z)	case x: valp = &rt_metrics.z; flag = y; break
	caseof(K_MTU, RTV_MTU, rmx_mtu);
	caseof(K_HOPCOUNT, RTV_HOPCOUNT, rmx_hopcount);
	caseof(K_EXPIRE, RTV_EXPIRE, rmx_expire);
	caseof(K_RECVPIPE, RTV_RPIPE, rmx_recvpipe);
	caseof(K_SENDPIPE, RTV_SPIPE, rmx_sendpipe);
	caseof(K_SSTHRESH, RTV_SSTHRESH, rmx_ssthresh);
	caseof(K_RTT, RTV_RTT, rmx_rtt);
	caseof(K_RTTVAR, RTV_RTTVAR, rmx_rttvar);
	}
	rtm_inits |= flag;
	if (lockrest || locking)
		rt_metrics.rmx_locks |= flag;
	if (locking)
		locking = 0;
	*valp = atoi(value);
	if (key == K_EXPIRE)
		*valp += time(NULL);
}

#define	NEXTARG {							\
	--argc;								\
	if (*++argv == NULL)						\
		usage(*--argv);						\
}

static int
newroute(int cmd, int argc, char *argv[])
{
	struct rt_msghdr *rtm;
	struct addrinfo *ai;
	int flags;
	int iflag;
	int key;
	int l;
	int rc;
	int seq;
	pid_t pid;
	char *dest;
	char *errmsg;
	char *gateway;

#define	rtmsize	(sizeof(*rtm) + 512)
	ai = NULL;
	dest = gateway = "";
	errmsg = NULL;
	iflag = 0;
	flags = RTF_STATIC;
	pid = getpid();
	if ((rtm = malloc(rtmsize)) == NULL)
		err(2, NULL);
	if (cmd != RTM_GET && uid != 0) {
		errno = EACCES;
		err(2, "must be root to alter routing table");
	}
	nflag = NI_NUMERICHOST;
#define	NEXTARG {							\
	--argc;								\
	if (*++argv == NULL)						\
		usage(*--argv);						\
}
	while (--argc > 0) {
		if (**(++argv)== '-') {
			switch ((key = keyword(*argv + 1))) {
			case K_LINK:
				af = AF_LINK;
				aflen = sizeof(struct sockaddr_dl);
				break;
#ifdef ISO
			case K_OSI:
			case K_ISO:
				af = AF_ISO;
				aflen = sizeof(struct sockaddr_iso);
				break;
#endif
			case K_INET:
				af = AF_INET;
				aflen = sizeof(struct sockaddr_in);
				break;
			case K_HTUNNEL:
				iflag++;  /* {host,router}-to-host tunnel. */
			case K_TUNNEL:
			case K_RTUNNEL:   /* {host,router}-to-router tunnel. */
				flags |= RTF_TUNNEL;
				break;
			case K_AUTH:	/* IPSEC */
				flags |= RTF_AUTH;
				break;
			case K_ENCRYPT:	/* IPSEC */
				flags |= RTF_CRYPT;
				break;
			case K_INET6:
				af = AF_INET6;
				aflen = sizeof(struct sockaddr_in6);
#if 0
				if (prefixlen("64") != 64) {
					fprintf(stderr, "internal error: "
						"setting prefixlen=64\n");
					exit(1);
				}
#endif
				break;
#ifdef X25
			case K_X25:
				af = AF_CCITT;
				aflen = sizeof(struct sockaddr_x25);
				break;
#endif
			case K_SA:
				af = PF_ROUTE;
				aflen = sizeof(sockunion);
				break;
#ifdef XNS
			case K_XNS:
				af = AF_NS;
				aflen = sizeof(struct sockaddr_ns);
				break;
#endif
			case K_IFACE:
			case K_INTERFACE:
				iflag++;
				/*FALLTHROUGH*/
			case K_NOSTATIC:
				flags &= ~RTF_STATIC;
				break;
			case K_LOCK:
				locking = 1;
				break;
			case K_LOCKREST:
				lockrest = 1;
				break;
			case K_HOST:
				if (forcenet)
					usage(*argv);
				forcehost++;
				break;
			case K_REJECT:
				flags |= RTF_REJECT;
				break;
			case K_BLACKHOLE:
				flags |= RTF_BLACKHOLE;
				break;
			case K_PROTO1:
				flags |= RTF_PROTO1;
				break;
			case K_PROTO2:
				flags |= RTF_PROTO2;
				break;
			case K_CLONING:
				flags |= RTF_CLONING;
				break;
			case K_CLONED:
				flags |= RTF_CLONED;
				break;
			case K_XRESOLVE:
				flags |= RTF_XRESOLVE;
				break;
			case K_STATIC:
				flags |= RTF_STATIC;
				break;
			case K_IFA:
				NEXTARG;
				getaddr(RTA_IFA, *argv, NULL);
				break;
			case K_IFP:
				NEXTARG;
				getaddr(RTA_IFP, *argv, NULL);
				break;
			case K_GENMASK:
				NEXTARG;
				getaddr(RTA_GENMASK, *argv, NULL);
				break;
			case K_GATEWAY:
				NEXTARG;
				getaddr(RTA_GATEWAY, *argv, &ai);
				break;
			case K_DST:
				NEXTARG;
				getaddr(RTA_DST, *argv, NULL);
				dest = *argv;
				break;
			case K_NETMASK:
				NEXTARG;
				getaddr(RTA_NETMASK, *argv, NULL);
				/* FALLTHROUGH */
			case K_NET:
				if (forcehost)
					usage(*argv);
				forcenet++;
				break;
			case K_PREFIXLEN:
				NEXTARG;
				if (prefixlen(*argv) == -1) {
					forcenet = 0;
					forcehost = 1;
				} else {
					forcenet = 1;
					forcehost = 0;
				}
				break;
			case K_MTU:
			case K_HOPCOUNT:
			case K_EXPIRE:
			case K_RECVPIPE:
			case K_SENDPIPE:
			case K_SSTHRESH:
			case K_RTT:
			case K_RTTVAR:
				NEXTARG;
				set_metric(*argv, key);
				break;
			default:
				usage(*argv);
			}
		} else {
			if ((rtm_addrs & RTA_DST) == 0) {
				dest = *argv;
				getaddr(RTA_DST, *argv, NULL);
				continue;
			} 
			if ((rtm_addrs & RTA_GATEWAY) == 0) {
				gateway = *argv;
				getaddr(RTA_GATEWAY, *argv, &ai);
				continue;
			}
#define	BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
			if (argc == 1 &&
			    (*argv)[1] == (char)0 && isdigit(**argv)) {

				if (**argv == '0') {
					warnx("old usage of trailing 0, "
					    "assuming route to interface");
					iflag = 1;
				} else {
					warnx("old usage of trailing digit, "
					    "assuming route via gateway");
					iflag = 0;
				}
				continue;
			}
#endif	/* BACKWARD_COMPATIBILITY */
			if (!forcehost && !netmask_specified) {
				getaddr(RTA_NETMASK, *argv, NULL);
				warnx("use -netmask to specify a netmask");
				forcenet++;
				continue;
			}
			usage(*argv);
		}
	}
	if (forcehost && (rtm_addrs & RTA_NETMASK))
		errx(2, "-host and -netmask (or prefix length) "
		    "are mutually exclusive");
	flags |= RTF_UP;
	if (!(rtm_addrs & RTA_NETMASK))
		flags |= RTF_HOST;
	if (iflag == 0)
		flags |= RTF_GATEWAY;

	/* Repeat request while we have gateways to try */
	while ((seq = rtmsg(rtm, cmd, flags)) == -1 && errno == ENETUNREACH) {
		if (ai == NULL || (ai = ai->ai_next) == NULL)
			break;
		memcpy(&so_gate, ai->ai_addr, ai->ai_addrlen);
	}

	if (seq > 0) {
		if (qflag || debugonly)
			return (0);
		do {
			if ((l = read(s, rtm, rtmsize)) < 0)
				err(2, "read from routing socket");
		} while (l > 0 && (rtm->rtm_seq != seq || rtm->rtm_pid != pid));
		if (cmd == RTM_GET)
			print_getmsg(rtm, l);
		else
			print_response(rtm, l, NULL);
		return (0);
	}

	rc = 2;
	switch (errno) {
	case ESRCH:
		/* Get/delete failed */
		rc = 1;
		errmsg = "entry not found";
		break;

	case EEXIST:
		/* Add failed, desk/mask pair exists */
		rc = 1;
		errmsg = "entry exists";
		break;

	case ENOBUFS:
		/* No memory for an entry */
		errmsg = "forwarding table is full";
		break;

	case EDQUOT:
		/* Change failed, no space for gateway */
		errmsg = "no space in entry";
		break;

	case EAFNOSUPPORT:
		/* Address family not supported */
		errmsg = "support for address family not available";
		break;

	case 0:
		rc = 0;
		break;

	default:
		/* Something else strange happened */
		errmsg = strerror(errno);
		break;
	}

	if (!qflag || rc != 1)
		print_response(rtm, rtm->rtm_msglen, errmsg);

	return (rc);
}

static void
inet_makenetandmask(u_int32_t net, struct sockaddr_in *sin)
{
	u_long mask;
	char *cp;

	rtm_addrs |= RTA_NETMASK;
	if (net == 0)			/* Default net gets default mask */
		mask = 0;
	else if (net > 0xffffff) {
		/* 
		 * Four octets specified (i.e. a normalized network),
		 * choose a mask so that no host bits are set.
		 */

		for (mask = IN_CLASSA_HOST; ; mask >>= 8)
			if ((net & mask) == 0)
				break;
		mask = ~mask;
	} else {
		/*
		 * Normalize the network assigning a mask to cover the
		 * specified octets.
		 */
		mask = 0xffffff;
		while ((net & 0xff000000) == 0) {
			net <<= 8;
			mask <<= 8;
		}
		/* Don't allow a netmask shorter than the class allows. */
		if (IN_CLASSB(net))
			mask |= IN_CLASSB_NET;
		else if (IN_CLASSC(net))
			mask |= IN_CLASSC_NET;
	}
	/* 
	 * Net 224 (and 224.0, 224.0.0, 224.0.0.0) is a special case,
	 * it means 224/4
	 */ 
	if (IN_MULTICAST(net) && (net & IN_CLASSD_HOST) == 0)
		mask = IN_CLASSD_NET;
	/* All other non-class A/B/C networks get host mask to be safe */
	else if (IN_MULTICAST(net) || IN_EXPERIMENTAL(net))
		mask = -1;

	/* Save the network */
	sin->sin_addr.s_addr = htonl(net);

	/* Save the mask */
	sin = &so_mask.sin;
	sin->sin_addr.s_addr = htonl(mask);
	sin->sin_family = 0;

	/* Calculate the length of the mask after trailing zeros are removed */
	sin->sin_len = 0;
	cp = (char *)(&sin->sin_addr + 1);
	while (*--cp == 0 && cp > (char *)sin)
		;
	sin->sin_len = 1 + cp - (char *)sin;
}

/*
 * Interpret an argument as a network address of some kind,
 * returning 1 if a host address, 0 if a network address.
 */
static void
getaddr(int which, char *s, struct addrinfo **aip)
{
	sockunion *su;
	struct addrinfo *ai;
	struct addrinfo req;
	struct netent *np;
	u_long val;
	int i;
	int tryhost;
	int trynet;
	char *c;
	char *ep;
	u_int8_t *p;

	tryhost = trynet = 0;

	/* Indicate that this address has been parsed */
	rtm_addrs |= which;

	/* Point to and initialize the sockaddr */
	switch (which) {
	case RTA_DST:
		su = &so_dst;
		su->sa.sa_family = af;
		if (!(trynet = forcenet) && !(tryhost = forcehost))
			tryhost = trynet = 1;
		break;

	case RTA_GATEWAY:
		su = &so_gate;
		su->sa.sa_family = af;
		tryhost = 1;
		break;

	case RTA_NETMASK:
		su = &so_mask;
		tryhost = trynet = 1;
		netmask_specified = 1;
		break;

	case RTA_GENMASK:
		su = &so_genmask;
		tryhost = trynet = 1;
		break;

	case RTA_IFP:
		su = &so_ifp;
		su->sdl.sdl_len = sizeof(*su);
		link_addr(s, &su->sdl);
		if (su->sdl.sdl_len == 0)
			err(1, "invalid ifp specification: %s");
		return;

	case RTA_IFA:
		su = &so_ifa;
		su->sa.sa_family = af;
		tryhost = 1;
		break;

	default:
		errx(2, "internal error in getaddr()");
		break;
	}

	/* Set the socket length */
	su->sa.sa_len = aflen;

	/* Handle the special case of ``default'' */
	if (strcmp(s, "default") == 0) {
		if (af == AF_UNSPEC) {
			su->sa.sa_family = af = AF_INET;
			su->sa.sa_len = aflen = sizeof(struct sockaddr_in);
		}
		switch (which) {
		case RTA_DST:
			forcenet++;
			getaddr(RTA_NETMASK, s, NULL);
			break;
		case RTA_NETMASK:
		case RTA_GENMASK:
			su->sa.sa_len = 0;
		}
		return;
	}

	if (strcmp(s, "transdefault") == 0) {
		switch (which) {
		case RTA_DST:
			forcenet++;
			su->sin6.sin6_len = sizeof(struct sockaddr_in6);
			/* Must have non-zero for this?!? */
			su->sin6.sin6_addr = in6addr_any;
			su->sin6.sin6_family = AF_INET6;
			getaddr(RTA_NETMASK, s, NULL);
			break;
		case RTA_NETMASK:
		case RTA_GENMASK:
			su->sin6.sin6_len = sizeof(struct sockaddr_in6);
			su->sin6.sin6_family = AF_INET6;
			su->sin6.sin6_addr = in6addr_tdmask;
			/* Kebe asks, do I necessarily want to force the */
			/* next address to be inet?  Good question. */
			if (1) {
				af = AF_INET;
				aflen = sizeof(struct sockaddr_in);
			}
			break;
		}
		return;
	}

	/* Special processing for various address families */
	switch (af) {
#ifdef XNS
	case AF_NS:
		if (which == RTA_DST) {
			struct sockaddr_ns *sms = &so_mask.sns;

			memset(sms, 0, sizeof(*sms));
			sms->sns_family = 0;
			sms->sns_len = 6;
			sms->sns_addr.x_net = *(union ns_net *)ns_bh;
			rtm_addrs |= RTA_NETMASK;
		}
		su->sns.sns_addr = ns_addr(s);
		return; /* XXX - (!ns_nullhost(su->sns.sns_addr)) */
#endif

#ifdef ISO
	case AF_OSI:
		su->siso.siso_addr = *iso_addr(s);
		if (which == RTA_NETMASK || which == RTA_GENMASK) {
			char *cp = (char *)TSEL(&su->siso);

			su->siso.siso_nlen = 0;
			do {--cp ;} while ((cp > (char *)su) && (*cp == 0));
			su->siso.siso_len = 1 + cp - (char *)su;
		}
		return;
#endif

	case AF_LINK:
		link_addr(s, &su->sdl);
		return;

#ifdef X25
	case AF_CCITT:
		ccitt_addr(s, &su->sx25);
		if (which == RTA_DST)
			x25_makemask();
		return;
#endif

	case PF_ROUTE:
		su->sa.sa_len = sizeof(*su);
		sockaddr(s, su);
		return;

	case AF_INET6:
	case AF_INET:
	case AF_UNSPEC:
		break;
	}

	if (aip != NULL)
		*aip = NULL;

	if ((c = strchr(s, '/')) != NULL) {
		if (which != RTA_DST)
			errx(2, "prefix lengths are only for prefixes");
		*(c++) = 0;
		if (forcehost)
			errx(2, "prefix lengths are not valid for hosts");
		tryhost = 0;
	}

	if (!tryhost && !trynet)
		errx(2, "internal error: host or network?");

	if (af == AF_UNSPEC || af == AF_INET) {
		/*
		 * Look up as a host if allowed.  If the first byte (basically
		 * the network number) is zero and this is not the default
		 * route and the -host keyword has not been explicitly given
		 * we'll fall through and try to parse it as a network.
		 */
		if (tryhost && inet_aton(s, &su->sin.sin_addr) != 0 &&
		    (!trynet ||
			inet_netof(su->sin.sin_addr) != INADDR_ANY ||
			su->sin.sin_addr.s_addr == INADDR_ANY)) {
			if (!trynet ||
			    inet_lnaof(su->sin.sin_addr) != INADDR_ANY) {
				su->sin.sin_family = af = AF_INET;
				su->sin.sin_len = aflen =
				    sizeof(struct sockaddr_in);
				return;
			}
			val = ntohl(su->sin.sin_addr.s_addr);
			goto netdone;
		}

		/* 
		 * If the result can be a network, try to look
		 * up as one We rely on the fact
		 * getnetbyname() will call inet_network().
		 */
		if (trynet &&
		    (np = getnetbyname(s)) != NULL && (val = np->n_net) != 0) {
		    netdone:
			if (which == RTA_DST)
				inet_makenetandmask(val, &su->sin);
			if (c == NULL) {
				su->sin.sin_family = af = AF_INET;
				su->sin.sin_len = aflen =
				    sizeof(struct sockaddr_in);
				return;
			}
			af = su->sin.sin_family = AF_INET;
			su->sin.sin_len = sizeof(su->sin);
			goto setmask;
		}
	}
	memset(&req, 0, sizeof(req));
	req.ai_family = af;
	if ((i = getaddrinfo(s, NULL, &req, &ai)))
		errx(2, "getaddrinfo: %s: %s",
		    s, gai_strerror(i));
	memcpy(&su->sa, ai->ai_addr, ai->ai_addrlen);
	af = ai->ai_addr->sa_family;

	if (af == AF_INET6 &&
	    (IN6_IS_ADDR_LINKLOCAL(&su->sin6.sin6_addr) ||
	    IN6_IS_ADDR_MC_LINKLOCAL(&su->sin6.sin6_addr)) &&
	    su->sin6.sin6_scope_id) {
		*(u_int16_t *)&su->sin6.sin6_addr.s6_addr[2] =
			htons(su->sin6.sin6_scope_id);
		su->sin6.sin6_scope_id = 0;
	}

	if (aip != NULL)
		*aip = ai;
	else
		freeaddrinfo(ai);

	if (c != NULL) {
		u_long mlen, max_mlen;

	setmask:
		switch (af) {
		case AF_INET6:
			max_mlen = 128;
			p = (u_int8_t *)&so_mask.sin6.sin6_addr;
			break;
		case AF_INET:
			max_mlen = 32;
			p = (u_int8_t *)&so_mask.sin.sin_addr;
			break;
		default:
			errx(2, "unable to generate prefix for %s addresses",
			    af_to_name(af));
		}

		mlen = strtoul(c, &ep, 10);
		if (*c == 0 || *ep != 0 ||
		    (mlen == ULONG_MAX && errno == EINVAL) || 
		    mlen == 0 || mlen > max_mlen)
			errx(2, "invalid prefix length: %s", c);

		rtm_addrs |= RTA_NETMASK;
		netmask_specified = 1;
		forcenet++;

		memset(&so_mask, 0, sizeof(so_mask));
		so_mask.sa.sa_len = su->sa.sa_len;
		so_mask.sa.sa_family = af;

		while (mlen >= 8) {
			*(p++) = 0xff;
			mlen -= 8;
		}

		while (mlen--)
			*p = (*p >> 1) | 0x80;
	}

	return;
}

#ifdef X25
static void
x25_makemask(void)
{
	char *cp;

	if ((rtm_addrs & RTA_NETMASK) == 0) {
		rtm_addrs |= RTA_NETMASK;
		for (cp = (char *)&so_mask.sx25.x25_net;
		     cp < &so_mask.sx25.x25_opts.op_flags; cp++)
			*cp = -1;
		so_mask.sx25.x25_len = 
		    (char *)&so_mask.sx25.x25_opts - (char *)&so_mask;
	}
	return;
}
#endif /* X25 */

static int
prefixlen(s)
	char *s;
{
	int len = atoi(s);
	int q;
	int r;
	char *p;
	int max;

	rtm_addrs |= RTA_NETMASK;
	switch (af) {
	case AF_INET6:
		max = 128;
		p = (char *)&so_mask.sin6.sin6_addr;
		break;
	case AF_INET:
		max = 32;
		p = (char *)&so_mask.sin.sin_addr;
		break;
	default:
		(void)fprintf(stderr, "prefixlen not supported in this af\n");
		exit(1);
		/*NOTREACHED*/
	}

	if (len < 0 || max < len) {
		(void)fprintf(stderr, "%s: bad value\n", s);
		exit(1);
	}

	q = len >> 3;
	r = len & 7;
	so_mask.sa.sa_family = af;
	so_mask.sa.sa_len = aflen;
	memset((void *)p, 0, max / 8);
	if (q > 0)
		memset((void *)p, 0xff, q);
	if (r > 0)
		*((u_char *)p + q) = (0xff00 >> r) & 0xff;
	if (len == max)
		return -1;
	else
		return len;
}

static int
get_prefixlen(sockunion *su)
{
	int plen;
	int max;
	u_char *p, c;

	switch (su->sa.sa_family) {
	case AF_INET6:
		max = 16;	/* 128 bits */
		p = (char *)&so_mask.sin6.sin6_addr;
		break;
#ifdef notdef
	case AF_INET:
		max = 4;	/* 32 bits */
		p = (char *)&so_mask.sin.sin_addr;
		break;
#endif
	default:
		return -1;
	}

	if (((u_char *)su) + su->sa.sa_len < p + max)
		max = (((u_char *)su) + su->sa.sa_len - p);

	plen = 0;
	while (*p == 0xff && max > 0) {
		p++;
		plen += 8;
		max--;
	}
	if (max > 0) {
		c = *p++;
		max--;
		while (c & 0x80) {
			c <<= 1;
			plen++;
		}
		if (c)
			return -1;
		while (max > 0) {
			if (*p)
				return -1;
			p++;
			max--;
		}
	}
	return plen;
}

#ifdef XNS
static char *
ns_print(struct sockaddr_ns *sns)
{
	static char mybuf[50], cport[10], chost[25];
	struct ns_addr work;
	union { union ns_net net_e; u_long long_e; } net;
	u_short port;
	u_char *q;
	char *host;
	char *p;

	host = "";
	work = sns->sns_addr;
	port = ntohs(work.x_port);
	work.x_port = 0;
	net.net_e  = work.x_net;
	if (ns_nullhost(work) && net.long_e == 0) {
		if (port == 0)
			return ("*.*");
		(void)sprintf(mybuf, "*.%XH", port);
		return (mybuf);
	}

	if (memcmp(ns_bh, work.x_host.c_host, 6) == 0)
		host = "any";
	else if (memcmp(ns_nullh, work.x_host.c_host, 6) == 0)
		host = "*";
	else {
		q = work.x_host.c_host;
		(void)sprintf(chost, "%02X%02X%02X%02X%02X%02XH",
		    q[0], q[1], q[2], q[3], q[4], q[5]);
		for (p = chost; *p == '0' && p < chost + 12; p++)
			;
		host = p;
	}
	if (port != 0)
		(void)sprintf(cport, ".%XH", htons(port));
	else
		*cport = 0;

	(void)sprintf(mybuf,"%XH.%s%s", ntohl(net.long_e), host, cport);
	return (mybuf);
}
#endif /* XNS */

static void
interfaces(void)
{
	struct rt_msghdr *rtm;
	size_t needed;
	int mib[6];
	char *buf;
	char *lim;
	char *next;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;		/* protocol */
	mib[3] = 0;		/* wildcard address family */
	mib[4] = NET_RT_IFLIST;
	mib[5] = 0;		/* no flags */
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		err(2, "route-sysctl-estimate");
	if ((buf = malloc(needed)) == NULL)
		err(2, "malloc");
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		err(2, "actual retrieval of interface table");
	lim = buf + needed;
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		print_rtmsg(rtm, rtm->rtm_msglen);
	}
}

static void
monitor(void)
{
	int n;
	char msg[2048];

	verbose = 1;
	if (debugonly) {
		interfaces();
		exit(0);
	}
	for (;;) {
		struct timeval tv;
		char tbuf[LINE_MAX];

		n = read(s, msg, sizeof(msg));
		(void)gettimeofday(&tv, NULL);
		(void)strftime(tbuf, sizeof(tbuf), "%D %T %Z",
		    localtime(&tv.tv_sec));
		warnx("got message of size %d at %s", n, tbuf);
		print_rtmsg((struct rt_msghdr *)msg, n);
	}
}

static int
rtmsg(struct rt_msghdr *rtm, int cmd, int flags)
{
	static int seq;
	int l;
	char *cp;

#define NEXTADDR(w, u) \
	if (rtm_addrs & (w)) { \
	    l = ROUNDUP(u.sa.sa_len); \
	    memmove(cp, &(u), l); cp += l; \
	}

	cp = (char *)(rtm + 1);
	errno = 0;
	memset(rtm, 0, sizeof(*rtm));
	if (cmd == RTM_GET && so_ifp.sa.sa_family == 0) {
		so_ifp.sa.sa_family = AF_LINK;
		so_ifp.sa.sa_len = sizeof(struct sockaddr_dl);
		rtm_addrs |= RTA_IFP;
	}
	rtm->rtm_type = cmd;
	rtm->rtm_flags = flags;
	rtm->rtm_version = RTM_VERSION;
	rtm->rtm_seq = ++seq;
	rtm->rtm_addrs = rtm_addrs;
	rtm->rtm_rmx = rt_metrics;
	rtm->rtm_inits = rtm_inits;

	if (rtm_addrs & RTA_NETMASK)
		mask_addr();
	NEXTADDR(RTA_DST, so_dst);
	NEXTADDR(RTA_GATEWAY, so_gate);
	NEXTADDR(RTA_NETMASK, so_mask);
	NEXTADDR(RTA_GENMASK, so_genmask);
	NEXTADDR(RTA_IFP, so_ifp);
	NEXTADDR(RTA_IFA, so_ifa);
	rtm->rtm_msglen = l = cp - (char *)rtm;
	if (verbose)
		print_rtmsg(rtm, l);
	if (!debugonly && write(s, rtm, l) < 0)
		return (-1);
	return (seq);
}

static void
mask_addr(void)
{
	int olen;
	char *cp1;
	char *cp2;

	olen = so_mask.sa.sa_len;
	cp1 = olen + (char *)&so_mask;
	for (so_mask.sa.sa_len = 0; cp1 > (char *)&so_mask; )
		if (*--cp1 != 0) {
			so_mask.sa.sa_len = 1 + cp1 - (char *)&so_mask;
			break;
		}
#ifdef ISO
	if ((rtm_addrs & RTA_DST) == 0)
		return;
	switch (so_dst.sa.sa_family) {
	case AF_NS:
	case AF_INET:
	case AF_CCITT:
	case AF_INET6:
	case 0:
		return;
	case AF_ISO:
		olen = MIN(so_dst.siso.siso_nlen,
			   MAX(so_mask.sa.sa_len - 6, 0));
		break;
	}
	cp1 = so_mask.sa.sa_len + 1 + (char *)&so_dst;
	cp2 = so_dst.sa.sa_len + 1 + (char *)&so_dst;
	while (cp2 > cp1)
		*--cp2 = 0;
	cp2 = so_mask.sa.sa_len + 1 + (char *)&so_mask;
	while (cp1 > so_dst.sa.sa_data)
		*--cp1 &= *--cp2;
	switch (so_dst.sa.sa_family) {
	case AF_ISO:
		so_dst.siso.siso_nlen = olen;
		break;
	}
#endif /* ISO */
}

static char *msgtypes[] = {
	"",
	"RTM_ADD: Add Route",
	"RTM_DELETE: Delete Route",
	"RTM_CHANGE: Change Metrics or flags",
	"RTM_GET: Report Metrics",
	"RTM_LOSING: Kernel Suspects Partitioning",
	"RTM_REDIRECT: Told to use different route",
	"RTM_MISS: Lookup failed on this address",
	"RTM_LOCK: fix specified metrics",
	"RTM_OLDADD: caused by SIOCADDRT",
	"RTM_OLDDEL: caused by SIOCDELRT",
	"RTM_RESOLVE: Route created by cloning",
	"RTM_NEWADDR: address being added to iface",
	"RTM_DELADDR: address being removed from iface",
	"RTM_IFINFO: iface status change",
	0,
};

static char *msgactions[] = {
	"",
	"add",
	"delete",
	"change",
	"get",
	"losing",
	"redirect",
	"miss",
	"lock",
	"oldadd",
	"olddel",
	"resolve",
	"newaddr",
	"deladdr",
	"ifinfo",
	NULL
};

static u_char metricnames[] =
"\011pksent\010rttvar\7rtt\6ssthresh\5sendpipe\4recvpipe\3expire\2hopcount\1mtu";
static u_char routeflags[] =
"\1UP\2GATEWAY\3HOST\4REJECT\5DYNAMIC\6MODIFIED\7DONE\010MASK_PRESENT\011CLONING\012XRESOLVE\013LLINFO\014STATIC\015BLACKHOLE\016CLONED\017PROTO2\020PROTO1\021DEFAULT\022ISAROUTER\023TUNNEL\024ENCRYPT\025AUTH";
static u_char ifnetflags[] =
"\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5PTP\6NOTRAILERS\7RUNNING\010NOARP\011PPROMISC\012ALLMULTI\013OACTIVE\014SIMPLEX\015LINK0\016LINK1\017LINK2\020MULTICAST";
static u_char addrnames[] =
"\1DST\2GATEWAY\3NETMASK\4GENMASK\5IFP\6IFA\7AUTHOR\010BRD";

static void
print_rtmsg(struct rt_msghdr *rtm, int msglen)
{
	struct if_msghdr *ifm;
	struct ifa_msghdr *ifam;

	if (!verbose)
		return;
	if (rtm->rtm_version != RTM_VERSION) {
		warnx("routing message version %d not understood",
		    rtm->rtm_version);
		return;
	}
	if (rtm->rtm_msglen > msglen) {
		warnx("message length mismatch, in packet %d, returned %d",
		    rtm->rtm_msglen, msglen);
	}
	
	(void)printf("%s: len %d, ", msgtypes[rtm->rtm_type], rtm->rtm_msglen);
	switch (rtm->rtm_type) {
	case RTM_IFINFO:
		ifm = (struct if_msghdr *)rtm;
		(void)printf("\tif# %d, flags: %s\n", 
		    ifm->ifm_index, bprintf(ifm->ifm_flags, ifnetflags));
		pmsg_addrs(ifm + 1, ifm->ifm_addrs);
		break;

	case RTM_NEWADDR:
	case RTM_DELADDR:
		ifam = (struct ifa_msghdr *)rtm;
		(void)printf("\tmetric %d, flags: %s\n", 
		    ifam->ifam_metric, bprintf(ifam->ifam_flags, routeflags));
		pmsg_addrs(ifam + 1, ifam->ifam_addrs);
		break;

	default:
		(void)printf("pid: %d, seq %d, errno %d",
		    rtm->rtm_pid, rtm->rtm_seq, rtm->rtm_errno);
		if (rtm->rtm_errno != 0)
			(void)printf(": %s", strerror(rtm->rtm_errno));
		(void)printf("\n\tflags: %s\n",
		    bprintf(rtm->rtm_flags, routeflags));
		pmsg_common(rtm);
		pmsg_addrs(rtm + 1, rtm->rtm_addrs);
		break;
	}
	(void)printf("\n");
	(void)fflush(stdout);
}

static void
print_response(struct rt_msghdr *rtm, int msglen, char *msg)
{
	int af;
	int is_default;
	sockinfo info;
	int plen;

	memset(&info, 0, sizeof(info));
	af = AF_INET;
	if (rtm->rtm_version != RTM_VERSION) {
		warnx("routing message version %d not understood",
		    rtm->rtm_version);
		return;
	}
	if (rtm->rtm_msglen > msglen) {
		warnx("message length mismatch, in packet %d, returned %d",
		    rtm->rtm_msglen, msglen);
	}
	xaddrs(rtm + 1, rtm->rtm_addrs, &info);

	(void)printf("%s -%s", msgactions[rtm->rtm_type], 
	    rtm->rtm_flags & RTF_HOST ? "host" : "net");
	if (info.rti_dst != NULL) {
		if (info.rti_dst->sa.sa_family != af) {
			af = info.rti_dst->sa.sa_family;
			(void)printf(" -%s", af_to_name(af));
		}
		(void)printf(" %s", 
		    routename(info.rti_dst, rtm->rtm_flags & RTF_HOST ? NULL :
			info.rti_netmask, 0, &is_default));
	}
	if (info.rti_netmask != NULL && !is_default) {
		if ((plen = get_prefixlen(info.rti_netmask)) > 0) {
			(void)printf(" -prefixlen %d", plen);
		} else {
			(void)printf(" -netmask %s", 
			    routename(info.rti_netmask, NULL, NI_NUMERICHOST,
			    NULL));
		}
	}
	if (info.rti_gateway != NULL && rtm->rtm_flags & RTF_GATEWAY) {
		if (info.rti_gateway->sa.sa_family != af) {
			af = info.rti_gateway->sa.sa_family;
			(void)printf(" -%s", af_to_name(af));
		}
		(void)printf(" %s", routename(info.rti_gateway, NULL, 0, NULL));
	}
	if (info.rti_ifa != NULL) {
		if (info.rti_ifa->sa.sa_family != af) {
			af = info.rti_ifa->sa.sa_family;
			(void)printf(" -%s", af_to_name(af));
		}
		(void)printf(" -ifa %s", routename(info.rti_ifa, NULL, 0, 
		    NULL));
	}
	if (info.rti_ifp != NULL)
		(void)printf(" -ifp %s", routename(info.rti_ifp, NULL, 0,
		    NULL));

	if (rtm->rtm_flags & RTF_STATIC)
		(void)printf(" -static");
	if (rtm->rtm_flags & RTF_TUNNEL)
		(void)printf(" -tunnel");
	if (rtm->rtm_flags & RTF_AUTH)
		(void)printf(" -auth");
	if (rtm->rtm_flags & RTF_CRYPT)
		(void)printf(" -encrypt");
	if (rtm->rtm_flags & RTF_REJECT)
		(void)printf(" -reject");
	if (rtm->rtm_flags & RTF_BLACKHOLE)
		(void)printf(" -blackhole");
	if (rtm->rtm_flags & RTF_PROTO1)
		(void)printf(" -proto1");
	if (rtm->rtm_flags & RTF_PROTO2)
		(void)printf(" -proto2");
	if (rtm->rtm_flags & RTF_XRESOLVE)
		(void)printf(" -xresolve");

	if (msg)
		(void)printf(": %s\n", msg);
	else
		(void)printf("\n");

	xaddrs_free(&info);
}

static void
print_getmsg(struct rt_msghdr *rtm, int msglen)
{
	int is_default;
	sockinfo info;

#define	FMT	"%11s: %s\n"

	memset(&info, 0, sizeof(info));
	is_default = 0;
	(void)printf(FMT, "route to", routename(&so_dst, &so_mask, 0, NULL));
	if (rtm->rtm_version != RTM_VERSION) {
		warnx("routing message version %d not understood",
		    rtm->rtm_version);
		return;
	}
	if (rtm->rtm_msglen > msglen) {
		warnx("message length mismatch, in packet %d, returned %d",
		    rtm->rtm_msglen, msglen);
	}
	if (rtm->rtm_errno != 0) {
		errno = rtm->rtm_errno;
		warnx("RTM_GET");
		return;
	}
	xaddrs(rtm + 1, rtm->rtm_addrs, &info);
	if (info.rti_dst != NULL)
		(void)printf(FMT, "destination", 
		    routename(info.rti_dst, 
			rtm->rtm_flags & RTF_HOST ? NULL : info.rti_netmask, 0,
			&is_default));
	if (info.rti_netmask != NULL && !is_default)
		(void)printf(FMT, "netmask", 
		    routename(info.rti_netmask, NULL, NI_NUMERICHOST, NULL));
	if (info.rti_gateway != NULL && rtm->rtm_flags & RTF_GATEWAY)
		(void)printf(FMT, "gateway",
		    routename(info.rti_gateway, NULL, 0, NULL));
	else if (info.rti_gateway != NULL && (info.rti_gateway->sdl.sdl_nlen ||
	    info.rti_gateway->sdl.sdl_alen))
		(void)printf(FMT, "link addr", 
		    routename(info.rti_gateway, NULL, 0, NULL));
	if (info.rti_ifp != NULL)
		(void)printf(FMT, "interface",
		    routename(info.rti_ifp, NULL, 0, NULL));
	if (info.rti_ifa != NULL)
		(void)printf(FMT, "if address",
		    routename(info.rti_ifa, NULL, 0, NULL));
	if (info.rti_brd != NULL)
		(void)printf(FMT, "broadcast", 
		    routename(info.rti_brd, NULL, 0, NULL));
	if (info.rti_author != NULL)
		(void)printf(FMT, "author",
		    routename(info.rti_author, NULL, 0, NULL));
	if (info.rti_genmask != NULL)
		(void)printf(FMT, "genmask",
		    routename(info.rti_genmask, NULL, NI_NUMERICHOST, NULL));
	(void)printf(FMT, "flags",
	    bprintf(rtm->rtm_flags, routeflags));

#define lock(f)	((rtm->rtm_rmx.rmx_locks & __CONCAT(RTV_,f)) ? 'L' : ' ')
#define msec(u)	(((u) + 500) / 1000)		/* usec to msec */

	(void)printf("%s\n", "\
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire");
	(void)printf("%8d%c ", rtm->rtm_rmx.rmx_recvpipe, lock(RPIPE));
	(void)printf("%8d%c ", rtm->rtm_rmx.rmx_sendpipe, lock(SPIPE));
	(void)printf("%8d%c ", rtm->rtm_rmx.rmx_ssthresh, lock(SSTHRESH));
	(void)printf("%8d%c ", msec(rtm->rtm_rmx.rmx_rtt), lock(RTT));
	(void)printf("%8d%c ", msec(rtm->rtm_rmx.rmx_rttvar), lock(RTTVAR));
	(void)printf("%8d%c ", rtm->rtm_rmx.rmx_hopcount, lock(HOPCOUNT));
	(void)printf("%8d%c ", rtm->rtm_rmx.rmx_mtu, lock(MTU));
	if (rtm->rtm_rmx.rmx_expire)
		rtm->rtm_rmx.rmx_expire -= time(NULL);
	(void)printf("%8d%c\n", rtm->rtm_rmx.rmx_expire, lock(EXPIRE));
#undef lock
#undef msec
	if (verbose)
		pmsg_common(rtm);
	xaddrs_free(&info);
#undef	FMT
}

static void
pmsg_common(struct rt_msghdr *rtm)
{
	char sep, term;

	if (rtm->rtm_rmx.rmx_locks != 0) {
		(void)printf("\tlocks: %s",
		    bprintf(rtm->rtm_rmx.rmx_locks, metricnames));
		sep = ' ';
		term = '\n';
	} else {
		sep = '\t';
		term = '\0';
	}

	if (rtm->rtm_inits != 0) {
		(void)printf("%cinits: %s", sep,
		    bprintf(rtm->rtm_inits, metricnames));
		term = '\n';
	}
	if (term != '\0')
		(void)printf("%c", term);
}

static void
pmsg_addrs(void *vp, int addrs)
{
	sockinfo info;
	int af;
	int i;

	af = AF_UNSPEC;
	if (addrs == 0)
		return;

	memset(&info, 0, sizeof(info));
	xaddrs(vp, addrs, &info);

	(void)printf("\taddrs:");
	for (i = 0; i < RTAX_MAX; i++) {
		if (((1 << i) & addrs) == 0)
			continue;
		(void)printf(" %s%s", bprintf(addrs & (1 << i), addrnames),
		    routename(info.rti_info[i], NULL, 0, NULL));
	}
	(void)printf("\n");

	xaddrs_free(&info);
}

static void
xaddrs(void *vp, int addrs, sockinfo *info)
{
	sockunion *sa, *sa2;
	int af;
	int i;
	size_t len;
	char *cp;

	af = AF_UNSPEC;
	len = 0;
	info->rti_addrs = addrs;

	for (cp = vp, i = 0; i < RTAX_MAX; i++) {
		if (((1 << i) & addrs) == 0)
			continue;
		sa = (sockunion *)cp;
		if (i == RTAX_DST || i == RTAX_IFA) {
			af = sa->sa.sa_family;
			len = sa->sa.sa_len;
			break;
		}
		ADVANCE(cp, sa);
	}
	for (cp = vp, i = 0; i < RTAX_MAX; i++) {
		if (((1 << i) & addrs) == 0)
			continue;
		sa = (sockunion *)cp;
		switch (i) {
		case RTAX_NETMASK:
		case RTAX_GENMASK:
			if ((sa2 = calloc(1, MAX(len, ROUNDUP(sa->sa.sa_len))))
			    == NULL)
				err(2, "malloc");
			memcpy(sa2, sa, ROUNDUP(sa->sa.sa_len));
			sa2->sa.sa_family = af;
			sa2->sa.sa_len = len;
			break;
				
		default:
			if ((sa2 = malloc(sa->sa.sa_len)) == NULL)
				err(2, "malloc");
			memcpy(sa2, sa, sa->sa.sa_len);
			break;
		}
		info->rti_info[i] = sa2;
		ADVANCE(cp, sa);
	}
}

static void
xaddrs_free(sockinfo *info)
{
	int i;

	for (i = 0; i < RTAX_MAX; i++) {
		if (info->rti_info[i] == NULL)
			continue;
		free(info->rti_info[i]);
		info->rti_info[i] = NULL;
	}
}

static char *
bprintf(int b, u_char *s)
{
	static char line[LINE_MAX];
	int i;
	char *cp;
	char sep;

	cp = line;
	*cp = 0;
	sep = '<';
	if (b == 0)
		return (line);
	while ((i = *s++)) {
		if (b & (1 << (i-1))) {
			*cp++ = sep;
			sep = ',';
			for (; (i = *s) > 32; s++)
				*cp++ = i;
		} else
			while (*s > 32)
				s++;
	}
	if (cp > line)
		*cp++ = '>';
	*cp = '\0';

	return (line);
}

static int
keyword(char *cp)
{
	struct keytab *kt;

	for (kt = keywords; kt->kt_cp != NULL && strcmp(kt->kt_cp, cp) != 0;
	     kt++)
		;

	return (kt->kt_i);
}

/* States*/
#define VIRGIN	0
#define GOTONE	1
#define GOTTWO	2
/* Inputs */
#define	DIGIT	(4*0)
#define	END	(4*1)
#define DELIM	(4*2)

static void
sockaddr(char *addr, sockunion *sa)
{
	int byte;
	int new;
	int size;
	int state;
	char *cp;
	char *cplim;

	byte = new = 0;
	state = VIRGIN;
	size = sa->sa.sa_len;
	cp = (char *)sa;
	cplim = cp + size;
	memset(cp, 0, size);
	cp++;
	do {
		if ((*addr >= '0') && (*addr <= '9')) {
			new = *addr - '0';
		} else if ((*addr >= 'a') && (*addr <= 'f')) {
			new = *addr - 'a' + 10;
		} else if ((*addr >= 'A') && (*addr <= 'F')) {
			new = *addr - 'A' + 10;
		} else if (*addr == 0)
			state |= END;
		else
			state |= DELIM;
		addr++;
		switch (state /* | INPUT */) {
		case GOTTWO | DIGIT:
			*cp++ = byte;
			/*FALLTHROUGH*/

		case VIRGIN | DIGIT:
			state = GOTONE; 
			byte = new; 
			continue;

		case GOTONE | DIGIT:
			state = GOTTWO; 
			byte = new + (byte << 4); 
			continue;

		default: /* | DELIM */
			state = VIRGIN;
			*cp++ = byte; 
			byte = 0; 
			continue;

		case GOTONE | END:
		case GOTTWO | END:
			*cp++ = byte; /* FALLTHROUGH */

		case VIRGIN | END:
			break;
		}
		break;
	} while (cp < cplim);
	sa->sa.sa_len = cp - (char *)sa;
}

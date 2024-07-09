/*	BSDI arp.c,v 2.20 2000/06/18 00:42:13 dab Exp	*/

/*
 * Copyright (c) 1984, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sun Microsystems, Inc.
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
"@(#) Copyright (c) 1984, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)arp.c	8.3 (Berkeley) 4/28/95";
#endif /* not lint */

/*
 * arp - display, set, and delete arp table entries
 */


#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_llc.h>
#include <net/if_token.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>

static int nflag;
static int s = -1;

int	delete __P((char *, char *));
void	dump __P((u_long));
void	link_print __P((u_char *));
int	file __P((char *));
void	get __P((char *));
void	getsocket __P((void));
int	rtmsg __P((int, int));
void	link_print_ether __P((u_char *const cp));
void	link_print_8025 __P((struct token_max_hdr *addr));
int	build_ether_llinfo __P((char *a, u_char *addr));
int	build_8025_llinfo __P((char *a, u_char *addr, int ifidx));
int	set __P((int, char **));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char **argv;
{
	enum { NONE, ARP_ADD, ARP_DUMP, ARP_DELETE, ARP_FILE } op;
	int ch;

	op = NONE;
	while ((ch = getopt(argc, argv, "afnds")) != EOF)
		switch (ch) {
		case 'a':
			if (op != NONE && op != ARP_DUMP)
				usage();
			op = ARP_DUMP;
			break;
		case 'd':
			if (op != NONE && op != ARP_DELETE)
				usage();
			op = ARP_DELETE;
			break;
		case 'f':
			if (op != NONE && op != ARP_FILE)
				usage();
			op = ARP_FILE;
			break;
		case 'n':
			nflag = 1;
			break;
		case 's':
			if (op != NONE && op != ARP_ADD)
				usage();
			op = ARP_ADD;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	switch (op) {
	case NONE:
		if (argc != 1)
			usage();
		get(argv[0]);
		break;
	case ARP_ADD:
		if (argc < 2 || argc > 5)
			usage();
		exit (set(argc, &argv[0]) ? 1 : 0);
	case ARP_DUMP:
		if (argc != 0)
			usage();
		dump(0);
		break;
	case ARP_DELETE:
		if (argc < 1 || argc > 2)
			usage();
		delete(argv[0], argv[1]);
		break;
	case ARP_FILE:
		if (argc != 1)
			usage();
		exit (file(argv[0]) ? 1 : 0);
	default:
		abort();
	}
	exit (0);
}

/*
 * Process a file to set standard arp entries
 */
int
file(name)
	char *name;
{
	FILE *fp;
	int i, retval;
	char line[100], arg[5][50], *args[5];

	if ((fp = fopen(name, "r")) == NULL) {
		fprintf(stderr, "arp: cannot open %s\n", name);
		exit(1);
	}
	args[0] = &arg[0][0];
	args[1] = &arg[1][0];
	args[2] = &arg[2][0];
	args[3] = &arg[3][0];
	args[4] = &arg[4][0];
	retval = 0;
	while(fgets(line, 100, fp) != NULL) {
		i = sscanf(line, "%s %s %s %s %s", arg[0], arg[1], arg[2],
		    arg[3], arg[4]);
		if (i < 2) {
			fprintf(stderr, "arp: bad line: %s\n", line);
			retval = 1;
			continue;
		}
		if (set(i, args))
			retval = 1;
	}
	fclose(fp);
	return (retval);
}

void
getsocket() {
	if (s < 0) {
		s = socket(PF_ROUTE, SOCK_RAW, 0);
		if (s < 0) {
			perror("arp: socket");
			exit(1);
		}
	}
}

struct	sockaddr_in so_mask = {8, 0, 0, { 0xffffffff}};
struct	sockaddr_inarp blank_sin = {sizeof(blank_sin), AF_INET }, sin_m;
struct	sockaddr_dl_8025 blank_sdl = {sizeof(blank_sdl), AF_LINK };
struct	sockaddr_dl_8025 sdl_m;

int	expire_time, flags, export_only, doing_proxy, found_entry;
struct	{
	struct	rt_msghdr m_rtm;
	char	m_space[512];
}	m_rtmsg;

/*
 * Set an individual arp entry 
 */
int
set(argc, argv)
	int argc;
	char **argv;
{
	struct hostent *hp;
	register struct sockaddr_inarp *sin = &sin_m;
	register struct sockaddr_dl *sdl;
	register struct rt_msghdr *rtm = &(m_rtmsg.m_rtm);
	int i, error;
	int len;
	u_char *llinfo;
	char *host = argv[0];
	char *laddr = argv[1];

	getsocket();
	argc -= 2;
	argv += 2;
	sdl_m = blank_sdl;
	sin_m = blank_sin;
	sin->sin_addr.s_addr = inet_addr(host);
	if (sin->sin_addr.s_addr == (u_int32_t)-1) {
		if (!(hp = gethostbyname(host))) {
			fprintf(stderr, "arp: %s: ", host);
			herror((char *)NULL);
			return (1);
		}
		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,
		    sizeof sin->sin_addr);
	}
	doing_proxy = flags = export_only = expire_time = 0;
	while (argc-- > 0) {
		if (strncmp(argv[0], "temp", 4) == 0) {
			struct timeval time;
			gettimeofday(&time, 0);
			expire_time = time.tv_sec + 20 * 60;
		}
		else if (strncmp(argv[0], "pub", 3) == 0) {
			flags |= RTF_ANNOUNCE;
			doing_proxy = SIN_PROXY;
		} else if (strncmp(argv[0], "trail", 5) == 0) {
			warnx("%s: Sending trailers is no longer supported",
				host);
		}
		argv++;
	}
tryagain:
	if (rtmsg(RTM_GET, 0) < 0) {
		perror(host);
		return (1);
	}
	sin = (struct sockaddr_inarp *)(rtm + 1);
	sdl = (struct sockaddr_dl *)(sin->sin_len + (char *)sin);
	if (sin->sin_addr.s_addr == sin_m.sin_addr.s_addr) {
		if (sdl->sdl_family == AF_LINK &&
		    (rtm->rtm_flags & RTF_LLINFO) &&
		    !(rtm->rtm_flags & RTF_GATEWAY)) 
			switch (sdl->sdl_type) {
			case IFT_ETHER: 
			case IFT_L2VLAN:
			case IFT_FDDI: 
			case IFT_ISO88023:
			case IFT_ISO88024: 
			case IFT_ISO88025:
			case IFT_IEEE80211:
				goto overwrite;
			}
		if (doing_proxy == 0) {
			warnx("host routing entry already exists, can only proxy for %s", host);
			return (1);
		}
		if (sin_m.sin_other & SIN_PROXY) {
			warnx("proxy entry already exists for non 802 device");
			return (1);
		}
		sin_m.sin_other = SIN_PROXY;
		export_only = 1;
		goto tryagain;
	}
overwrite:
	if (sdl->sdl_family != AF_LINK) {
		printf("cannot intuit interface index and type for %s\n", host);
		return (1);
	}
	llinfo = (u_char *)LLADDR(&sdl_m);
	switch (sdl->sdl_type) {
	case IFT_ETHER:
	case IFT_L2VLAN:
	case IFT_FDDI:
	case IFT_IEEE80211:
		len = build_ether_llinfo(laddr, llinfo);
		break;
	case IFT_ISO88025:
		len = build_8025_llinfo(laddr, llinfo, sdl->sdl_index);
		break;
	default:
		printf("Interface type (%d) does not support ARP\n",
		    sdl->sdl_type);
		return (1);
	}
	if (len < 0)
		return (1);
	sdl_m.sdl_type = sdl->sdl_type;
	sdl_m.sdl_alen = len;
	sdl_m.sdl_len = sizeof(sdl_m) - sizeof(sdl_m.sdl_data) + len;
	if (sdl_m.sdl_len < sizeof(struct sockaddr_dl))
		sdl_m.sdl_len = sizeof(struct sockaddr_dl);
	sdl_m.sdl_index = sdl->sdl_index;

	/*
	 * Try the addition several times, in case the kernel creates
	 * an incomplete route before we start or while we work.
	 */
	for (i = 3; i > 0; i--) {
		if ((error = rtmsg(RTM_ADD, 1)) == 0)
			break;
		/* delete incomplete route and retry */
		(void) rtmsg(RTM_DELETE, 1);
	}
	if (error)
		error = rtmsg(RTM_ADD, 0);	/* final attempt */
	return (error);
}

/*
 * Display an individual arp entry
 */
void
get(host)
	char *host;
{
	struct hostent *hp;
	struct sockaddr_inarp *sin = &sin_m;

	sin_m = blank_sin;
	sin->sin_addr.s_addr = inet_addr(host);
	if (sin->sin_addr.s_addr == (u_int32_t)-1) {
		if (!(hp = gethostbyname(host))) {
			fprintf(stderr, "arp: %s: ", host);
			herror((char *)NULL);
			exit(1);
		}
		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,
		    sizeof sin->sin_addr);
	}
	dump(sin->sin_addr.s_addr);
	if (found_entry == 0) {
		printf("%s (%s) -- no entry\n",
		    host, inet_ntoa(sin->sin_addr));
		exit(1);
	}
}

/*
 * Delete an arp entry 
 */
int
delete(host, info)
	char *host;
	char *info;
{
	struct hostent *hp;
	register struct sockaddr_inarp *sin = &sin_m;
	register struct rt_msghdr *rtm = &m_rtmsg.m_rtm;
	struct sockaddr_dl *sdl;

	if (info && strncmp(info, "pro", 3) )
		export_only = 1;
	getsocket();
	sin_m = blank_sin;
	sin->sin_addr.s_addr = inet_addr(host);
	if (sin->sin_addr.s_addr == (u_int32_t)-1) {
		if (!(hp = gethostbyname(host))) {
			fprintf(stderr, "arp: %s: ", host);
			herror((char *)NULL);
			return (1);
		}
		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,
		    sizeof sin->sin_addr);
	}
tryagain:
	if (rtmsg(RTM_GET, 0) < 0) {
		perror(host);
		return (1);
	}
	sin = (struct sockaddr_inarp *)(rtm + 1);
	sdl = (struct sockaddr_dl *)(sin->sin_len + (char *)sin);
	if (sin->sin_addr.s_addr == sin_m.sin_addr.s_addr) {
		if (sdl->sdl_family == AF_LINK &&
		    (rtm->rtm_flags & RTF_LLINFO) &&
		    !(rtm->rtm_flags & RTF_GATEWAY)) 
			switch (sdl->sdl_type) {
			case IFT_ETHER: 
			case IFT_L2VLAN:
			case IFT_FDDI:
			case IFT_ISO88023:
			case IFT_ISO88024: 
			case IFT_ISO88025:
			case IFT_IEEE80211:
				goto delete;
			}
	}
	if (sin_m.sin_other & SIN_PROXY) {
		fprintf(stderr, "delete: can't locate %s\n",host);
		return (1);
	} else {
		sin_m.sin_other = SIN_PROXY;
		goto tryagain;
	}
delete:
	if (sdl->sdl_family != AF_LINK) {
		printf("cannot locate %s\n", host);
		return (1);
	}
	if (rtmsg(RTM_DELETE, 0))
		return (1);
	printf("%s (%s) deleted\n", host, inet_ntoa(sin->sin_addr));
	return (0);
}

/*
 * Dump the entire arp table
 */
void
dump(addr)
	u_long addr;
{
	int mib[6];
	size_t needed;
	char *host, *lim, *buf, *next;
	struct rt_msghdr *rtm;
	struct sockaddr_inarp *sin;
	struct sockaddr_dl *sdl;
	extern int h_errno;
	struct hostent *hp;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = AF_INET;
	mib[4] = NET_RT_FLAGS;
	mib[5] = RTF_LLINFO;
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		err(1, "route-sysctl-estimate");
	if ((buf = malloc(needed)) == NULL)
		err(1, "malloc");
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		err(1, "actual retrieval of routing table");
	lim = buf + needed;
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		sin = (struct sockaddr_inarp *)(rtm + 1);
		sdl = (struct sockaddr_dl *)(sin + 1);
		if (addr) {
			if (addr != sin->sin_addr.s_addr)
				continue;
			found_entry = 1;
		}
		if (nflag == 0)
			hp = gethostbyaddr((caddr_t)&(sin->sin_addr),
			    sizeof sin->sin_addr, AF_INET);
		else
			hp = NULL;
		if (hp != NULL)
			host = hp->h_name;
		else {
			host = "?";
			if (h_errno == TRY_AGAIN)
				nflag = 1;
		}
		printf("%s (%s) at ", host, inet_ntoa(sin->sin_addr));
		if (sdl->sdl_alen) {
			switch (sdl->sdl_type) {
			case IFT_ETHER:
			case IFT_L2VLAN:
			case IFT_FDDI:
			case IFT_IEEE80211:
				link_print_ether((u_char *)LLADDR(sdl));
				break;
			case IFT_ISO88025:
				link_print_8025((struct token_max_hdr *)
				    LLADDR(sdl));
				break;
			default:
			    {
				char *cp, *cp2;
				cp = link_ntoa(sdl);
				/* Convert '.' to ':' */
				for (cp2 = cp; *cp2; cp2++)
					if (*cp2 == '.')
						*cp2 = ':';
				printf("%s", cp);
				break;
			    }
			}
			if (rtm->rtm_rmx.rmx_expire == 0)
				printf(" permanent");
		} else
			printf("(incomplete)");
		if (sin->sin_other & SIN_PROXY)
			printf(" published (proxy only)");
		if (rtm->rtm_addrs & RTA_NETMASK) {
			sin = (struct sockaddr_inarp *)
				(sdl->sdl_len + (char *)sdl);
			if (sin->sin_addr.s_addr == 0xffffffff)
				printf(" published");
			if (sin->sin_len != 8)
				printf("(wierd)");
		}
		printf("\n");
	}
}

/*
 * Display an ethernet address
 *	xx:xx:xx:xx:xx:xx
 */
void
link_print_ether(u_char *const cp)
{
	printf("%x:%x:%x:%x:%x:%x", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
	return;
}

/*
 * Display an 802.5 address with optional source routing info:
 *	xx:xx:xx:xx:xx:xx/f|r:nnnn:nnnn,... (up to 8 segments)
 */
void
link_print_8025(struct token_max_hdr *addr)
{
	int nseg;
	int i;

	printf("%x:%x:%x:%x:%x:%x", 
		addr->hdr.token_dhost[0], addr->hdr.token_dhost[1],
		addr->hdr.token_dhost[2], addr->hdr.token_dhost[3],
		addr->hdr.token_dhost[4], addr->hdr.token_dhost[5]);
	if (!HAS_ROUTE(addr))
		return;
	nseg = addr->rif.rcf0 & RCF0_LEN_MASK;
	if (nseg <= 2) 
		return;
	nseg = (nseg >> 1) - 1;
	if (addr->rif.rcf1 & RCF1_DIRECTION)
		printf("/r");
	else
		printf("/f");
	for (i = 0; i < nseg; i++)
		printf(":%x", ntohs(addr->rif.rseg[i]));
}

/*
 * Parse an ethernet link address
 */
int
build_ether_llinfo(char *a, u_char *addr)
{
	char *nxt = a;
	int i;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		a = strsep(&nxt, ":/");
		if (!a) {
			fprintf(stderr,"Short 802 MAC address\n");
			return (-1);
		}
		addr[i] = strtoul(a, NULL, 16);
	}
	a = strsep(&nxt, ":");
	if (a) {
		fprintf(stderr, "Invalid MAC address\n");
		return (-1);
	}
	return (i);
}

/*
 * Build an 802.5 (token ring) arp llinfo. A token ring arp entry contains
 * the entire packet header, optional routing info, and an IP LLC/SNAP header.
 */
int
build_8025_llinfo(char *a, u_char *addr, int ifidx)
{
	char *nxt = a;
	struct token_max_hdr *hp = (struct token_max_hdr *)addr;
	int i;
	struct ifaddrs *ifap;
	struct ifaddrs *ifaddrs;
	char *my_link_addr = NULL;
	struct llc *llcp;

	/*
	 * First trick is to find our MAC address on the selected interface;
	 * ARP entries for 802.5 contain the entire header prebuilt. This
	 * entails getting details on all interfaces and finding the
	 * link address for the one that was selected for this arp entry.
	 */
	if (getifaddrs(&ifaddrs) < 0)
		err(1, "getifaddrs");

	for (ifap = ifaddrs; ifap != NULL; ifap = ifap->ifa_next) {
#define drp ((struct sockaddr_dl *)ifap->ifa_addr)
		if (ifap->ifa_addr != NULL &&
		    drp->sdl_family == AF_LINK &&
		    drp->sdl_index == ifidx &&
		    drp->sdl_type == IFT_ISO88025 &&	/* a little paranoia */
		    drp->sdl_alen == ISO88025_ADDR_LEN) {
			my_link_addr = drp->sdl_data + drp->sdl_nlen;
			break;
		}
#undef drp
	}
	freeifaddrs(ifaddrs);
	if (my_link_addr == NULL) {
		fprintf(stderr, "Couldn't find interface address\n");
		return (-1);
	}

	/* Fill in the header */
	bcopy(my_link_addr, hp->hdr.token_shost, ISO88025_ADDR_LEN);
	hp->hdr.token_acf = ACF_PRIORITY3;
	hp->hdr.token_fcf = FCF_LLC_FRAME;

	/* Now parse the basic MAC address */
	for (i = 0; i < ISO88025_ADDR_LEN; i++) {
		a = strsep(&nxt, ":/");
		if (!a) {
			fprintf(stderr,"Short 802 MAC address\n");
			return (-1);
		}
		hp->hdr.token_dhost[i] = strtoul(a, NULL, 16);
	}
	a = strsep(&nxt, ":");
	if (!a) {
		/* No routing info, go tack on LLC/SNAP header */
		llcp = (struct llc *)&hp->rif;
		goto done;
	}

	/*
	 * User has routing info
	 */
	hp->hdr.token_shost[0] |= RI_PRESENT;
	hp->rif.rcf1 = RCF1_FRAME2;
	switch (*a) {
	case 'r':
		hp->rif.rcf1 |= RCF1_DIRECTION;
		break;
	case 'f':
		break;
	default:
		fprintf(stderr,"Invalid 802.5 direction designator: %c\n", *a);
		return (-1);
	}
	i = 0;
	while ((a = strsep(&nxt, ":")) != NULL) {
		if (!a)
			break;
		if (i >= 8)
			break;
		hp->rif.rseg[i++] = htons(strtoul(a, NULL, 16));
	}
	if (!i) {
		fprintf(stderr,"No 802.5 source route segments specified\n");
		return (-1);
	}
	if (i > 8) {
		fprintf(stderr,"Too many 802.5 route segments, 8 max\n");
		return (-1);
	}
	hp->rif.rcf0 = ((i + 1) << 1) & RCF0_LEN_MASK;
	llcp = (struct llc *)&hp->rif.rseg[i];

done:
	llcp->llc_dsap = LLC_SNAP_LSAP;
	llcp->llc_ssap = LLC_SNAP_LSAP;
	llcp->llc_control = LLC_UI;
	bzero(llcp->llc_org_code, 3);
	llcp->llc_ether_type = htons(ETHERTYPE_IP);

	return((char *)llcp - (char *)hp + LLC_SNAPLEN);
}

void
usage()
{
	fprintf(stderr, "usage: arp [-n] hostname\n");
	fprintf(stderr, "       arp -a [-n]\n");
	fprintf(stderr, "       arp -d hostname\n");
	fprintf(stderr, "       arp -f filename\n");
	fprintf(stderr, "       arp -s hostname link_addr [temp] [pub]\n");
	fprintf(stderr, "	link_addr = m:m:m:m:m:m[/f|r:seg1[:...]]\n");
	exit(1);
}

int
rtmsg(cmd, quiet)
	int cmd, quiet;
{
	static int seq;
	pid_t pid;
	int rlen;
	register struct rt_msghdr *rtm = &m_rtmsg.m_rtm;
	register char *cp = m_rtmsg.m_space;
	register int l;

	pid = getpid();
	errno = 0;
	if (cmd == RTM_DELETE)
		goto doit;
	bzero((char *)&m_rtmsg, sizeof(m_rtmsg));
	rtm->rtm_flags = flags;
	rtm->rtm_version = RTM_VERSION;

	switch (cmd) {
	default:
		fprintf(stderr, "arp: internal wrong cmd\n");
		exit(1);
	case RTM_ADD:
		rtm->rtm_addrs |= RTA_GATEWAY;
		rtm->rtm_rmx.rmx_expire = expire_time;
		rtm->rtm_inits = RTV_EXPIRE;
		rtm->rtm_flags |= (RTF_HOST | RTF_STATIC);
		sin_m.sin_other = 0;
		if (doing_proxy) {
			if (export_only)
				sin_m.sin_other = SIN_PROXY;
			else {
				rtm->rtm_addrs |= RTA_NETMASK;
				rtm->rtm_flags &= ~RTF_HOST;
			}
		}
		/* FALLTHROUGH */
	case RTM_GET:
		rtm->rtm_addrs |= RTA_DST;
		break;
	}
#define NEXTADDR(w, s, l) \
	if (rtm->rtm_addrs & (w)) { \
		bcopy((char *)&s, cp, sizeof(s)); cp += (l);}

	NEXTADDR(RTA_DST, sin_m, sizeof(sin_m));
	NEXTADDR(RTA_GATEWAY, sdl_m, sdl_m.sdl_len);
	NEXTADDR(RTA_NETMASK, so_mask, sizeof(so_mask));

	rtm->rtm_msglen = cp - (char *)&m_rtmsg;
doit:
	l = rtm->rtm_msglen;
	rtm->rtm_seq = ++seq;
	rtm->rtm_type = cmd;
	if ((rlen = write(s, (char *)&m_rtmsg, l)) < 0) {
		if (errno != ESRCH || cmd != RTM_DELETE) {
			if (!quiet)
				perror("writing to routing socket");
			return (-1);
		}
	}
	do {
		l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
	} while (l > 0 && (rtm->rtm_seq != seq || rtm->rtm_pid != pid));
	if (l < 0)
		(void) fprintf(stderr, "arp: read from routing socket: %s\n",
		    strerror(errno));
	return (0);
}

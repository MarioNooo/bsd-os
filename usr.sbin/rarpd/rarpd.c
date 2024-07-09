/*	BSDI	rarpd.c,v 2.7 2000/03/03 20:24:01 jch Exp	*/

/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
char    copyright[] =
"@(#) Copyright (c) 1990 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif				/* not lint */

#ifndef lint
static char rcsid[] =
"@(#) rarpd.c,v 2.7 2000/03/03 20:24:01 jch Exp";
#endif


/*
 * rarpd - Reverse ARP Daemon
 *
 * Usage:	rarpd -a [ -d -f ]
 *		rarpd [ -d -f ] interface
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_vlan.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>

#include <dirent.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#ifndef TFTP_DIR
#define TFTP_DIR "/tftpboot"
#endif

/*
 * The structure for each address.
 */
struct addr_info {
	TAILQ_ENTRY(addr_info) ai_link;
	struct	in_addr ai_ipaddr;	/* IP address of interface */
	struct	in_addr ai_netmask;	/* Netmask of interface */
};
#define	ADDR_LIST(ii, ai)						\
	{								\
		register struct addr_info *Xai;				\
		for (Xai = (ii)->ii_alist.tqh_first; ((ai) = Xai);) {	\
			Xai = Xai->ai_link.tqe_next;
#define	ADDR_LIST_END(ii, ai)						\
		}							\
	}

/*
 * The structure for each interface.
 */
struct if_info {
	TAILQ_ENTRY(if_info) ii_link;
	int     ii_fd;			/* BPF file descriptor */
	int	ii_type;		/* Link type */
	int	ii_tci;			/* 802.1Q VLAN TCI */
	char	*ii_ifname;		/* Interface name */
	u_char  ii_eaddr[6];		/* Ethernet address of this interface */
	TAILQ_HEAD(addr_head, addr_info) ii_alist;
};
#define	IF_LIST(ii)							\
	{								\
		register struct if_info *Xii;				\
		for (Xii = iflist.tqh_first; ((ii) = Xii);) {		\
			Xii = Xii->ii_link.tqe_next;
#define	IF_LIST_END(ii)							\
		}							\
	 }

TAILQ_HEAD(if_head, if_info);
static struct if_head iflist;

static int	bpf_open __P((void));
static void	debug __P((const char *,...));
static void	interface_scan __P((void));
static void	print_arp __P((struct if_info *, char *, int, void *));
static int	rarp_bootable __P((struct in_addr));
static int	rarp_check __P((struct if_info *, void *, int));
static int	rarp_open __P((struct ifaddrs *));
static void	rarp_loop __P((void));
static void	rarp_process __P((struct if_info *, void *));
static void	rarp_reply __P((struct if_info *, struct addr_info *,
		    void *, struct in_addr));
static void	rarp_request __P((struct if_info *));
static void	rarp_response __P((struct if_info *, void *));
static void	serr __P((int, const char *, ...));
static void	update_arptab __P((u_char *, struct in_addr));
static void	usage __P((void));
static void	handler __P((int));

extern int ether_ntohost(char *, u_char *);


static DIR	*dir_handle;		/* For reading directory */

static int	rescan;			/* Time to rescan interfaces */
static int	aflag;			/* listen on "all" interfaces  */
static int 	dflag;			/* print debugging messages */
static int 	fflag;			/* don't fork */
static int	rarpd;			/* True for rarpd, False for rarp */
static int 	Aflag;			/* Respond to All rarp requests */

static char	*ifname;		/* Configured interface name */
static char	*ifaddr;		/* Configured interface address */
static char	*progname;

int
main(argc, argv)
	int     argc;
	char  **argv;
{
	struct itimerval itv;
	int op;
	char *ep;

	if ((progname = strrchr(argv[0], '/')))
		++progname;
	else
		progname = argv[0];
	if (*progname == '-')
		++progname;

	rarpd = strcmp(progname, "rarp");

	/* Set timer defaults */
	timerclear(&itv.it_value);
	timerclear(&itv.it_interval);
	if (rarpd)
		itv.it_value.tv_sec = 5 * 60;
	else
		itv.it_value.tv_sec = 5;

	while ((op = getopt(argc, argv, rarpd ? "adft:A" : "dt:A")) != EOF) {
		switch (op) {
		case 'a':
			++aflag;
			break;

		case 'd':
			++dflag;
			break;

		case 'f':
			++fflag;
			break;

		case 't':
			itv.it_value.tv_sec = strtoul(optarg, &ep, 10);
			if (*optarg == 0 || *ep != 0 ||
			    (itv.it_value.tv_sec == (long)ULONG_MAX && errno == ERANGE))
			    usage();
			break;
			
		case 'A':
			++Aflag;
			break;

		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	ifname = NULL;
	ifaddr = NULL;
	switch (argc) {
	case 0:
		if (!aflag || !rarpd)
			usage();
		break;
		
	case 1:
		ifname = *argv;
		break;

	case 2:
		if (aflag || !rarpd)
			usage();
		ifname = *argv++;
		ifaddr = *argv;
		break;

	default:
		usage();
		/*NOTREACHED*/
	}

	/* All error reporting is done through syslogs. */
	openlog(progname, LOG_PID | LOG_CONS | (dflag ? LOG_PERROR : 0),
	    LOG_DAEMON);

	if (rarpd && !Aflag) {
		dir_handle = opendir(TFTP_DIR);
		if (dir_handle == NULL)
			serr(LOG_ERR, "Can not open dir %s: %s",
			    TFTP_DIR, strerror(errno));
	}

	TAILQ_INIT(&iflist);
	interface_scan();

	if (!rarpd) {
		if (iflist.tqh_first == NULL)
			serr(LOG_ERR, "no interfaces");
		rarp_request(iflist.tqh_first);
	} else if (!fflag && !dflag)
		daemon(0, 0);

	(void)signal(SIGALRM, handler);
	(void)signal(SIGTERM, handler);
	(void)signal(SIGINT, handler);
	(void)signal(SIGHUP, handler);

	if (rarpd)
	    itv.it_interval = itv.it_value;
	if (setitimer(ITIMER_REAL, &itv, NULL) < 0)
		serr(LOG_ERR, "setting timeout");

	rarp_loop();

	/*NOTREACHED*/
	return 1;
}

/*
 * Scan the interface list and locate all IP addresses on all Ethernet
 * interfaces.
 */
void
interface_scan()
{
	struct hostent *hp;
	struct in_addr in;
	struct if_info *ii;
	struct addr_info *ai;
	struct sockaddr_dl *sdl;
	struct sockaddr_in *sinp;
	struct ifaddrs *ifaddrs, *ifap;
	static int s = -1;
	u_int tci;
	char **ap;

	if (s == -1) {
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s == -1)
			serr(LOG_ERR, "unable to open UDP socket: %s",
			    strerror(errno));
	}

	if (ifaddr) {
		hp = gethostbyname(ifaddr);
		if (hp == NULL)
			serr(LOG_ERR, "unable to resolve %s: %s",
			    ifaddr, hstrerror(h_errno));
	} else
		hp = NULL;

	if (getifaddrs(&ifaddrs) < 0)
		serr(LOG_ERR, "unable to get interface addresses");

	/* Remove all addresses */
	IF_LIST(ii) {
		ADDR_LIST(ii, ai) {
			TAILQ_REMOVE(&ii->ii_alist, ai, ai_link);
			free(ai);
		} ADDR_LIST_END(ii, ai) ;
	} IF_LIST_END(ii) ;

	ai = NULL;
	ii = NULL;
	sdl = NULL;
	for (ifap = ifaddrs; ifap != NULL; ifap = ifap->ifa_next) {

		switch (ifap->ifa_addr->sa_family) {
		case AF_LINK:
			/* New interface */

			ii = NULL;		   
			sdl = (struct sockaddr_dl *)ifap->ifa_addr;

			/*
			 * Ignore loopback, P2P interfaces and
			 * interfaces that are not up.
			 */
			if ((ifap->ifa_flags &
			    (IFF_LOOPBACK | IFF_POINTOPOINT)) != 0 ||
			    (ifap->ifa_flags & IFF_UP) == 0) {
				sdl = NULL;
				continue;
			}
			/* We only work on Ethernet at the moment */
			switch (sdl->sdl_type) {
				struct vlanreq vreq;

				/* 
				 * These must match switch statement in
				 * rarp_open();
				 */
			case IFT_L2VLAN:
				strncpy(vreq.vlr_name, ifap->ifa_name, 
				    sizeof (vreq.vlr_name));
				if (ioctl(s, SIOCGETVLAN, (caddr_t)&vreq) < 0)
					serr(LOG_ERR, 
					    "Unable to get VLAN info for %s: %s");
				tci = EVLQ_TCI(0, 0, vreq.vlr_id);
				break;

			case IFT_ETHER:
				tci = 0;
				break;

			default:
				sdl = NULL;
				continue;
			}
			/* Are we looking for a specific interface? */
			if (ifname != NULL &&
			    strcmp(ifname, ifap->ifa_name) != 0) {
				/*
				 * If we are looking for all addresses
				 * on the specified interface, then if
				 * any address has been found we are
				 * finished.  Note that if we were
				 * looking for the primary, or a
				 * specific address, we would never
				 * get this far.
				 */
				if (ai != NULL)
					goto Return;
				sdl = NULL;
				continue;
			}

			/* Look for an existing entry */
			IF_LIST(ii) {
				if (strcmp(ii->ii_ifname, ifap->ifa_name) == 0)
					break;
			} IF_LIST_END(ii) ;

			/* Verify that an existing entry is valid */
			if (ii != NULL) {
				if (ii->ii_type == sdl->sdl_type &&
				    ii->ii_tci == tci)
					break;

				/* Remove old entry */
				TAILQ_REMOVE(&iflist, ii, ii_link);
				free(ii->ii_ifname);
				(void)close(ii->ii_fd);
				free(ii);
				ii = NULL;
			}

			/* Create an entry if one does not exist */
			if (ii != NULL)
				break;

			ii = (struct if_info *)malloc(sizeof(*ii));
			if (ii == 0)
				serr(LOG_ERR, "malloc: %s",
				    strerror(errno));
			TAILQ_INSERT_TAIL(&iflist, ii, ii_link);
			TAILQ_INIT(&ii->ii_alist);
			memcpy(ii->ii_eaddr, LLADDR(sdl), sizeof ii->ii_eaddr);
			ii->ii_ifname = strdup(ifap->ifa_name);
			ii->ii_fd = rarp_open(ifap);
			ii->ii_type = sdl->sdl_type;
			ii->ii_tci = tci;

			if (!rarpd)
				goto Return;
			break;

		case AF_INET:
			/* Verify that this interface is supported */
			if (!rarpd || sdl == NULL || ii == NULL)
				continue;

			sinp = (struct sockaddr_in *)ifap->ifa_addr;

			/* If an intf addr was specified, check for it */
			if (hp != NULL) {
				for (ap = hp->h_addr_list; *ap != NULL; ap++) {
					memcpy(&in.s_addr, *ap, hp->h_length);
					if (in.s_addr == sinp->sin_addr.s_addr)
						break;
				}
				if (*ap == NULL)
					continue;
			}
			
			ai = (struct addr_info *)malloc(sizeof(*ai));
			if (ai == NULL)
				serr(LOG_ERR, "malloc: %s",
				    strerror(errno));
			TAILQ_INSERT_TAIL(&ii->ii_alist, ai, ai_link);
			ai->ai_ipaddr = sinp->sin_addr;
			sinp = (struct sockaddr_in *)ifap->ifa_netmask;
			ai->ai_netmask = sinp->sin_addr;

			/*
			 * If an interface name was specified and we
			 * are looking for a specific address, or the
			 * primary address, then we are finished.
			 */
			if (ifname != NULL && !aflag)
				goto Return;
			break;

		default:
			break;
		}
	}

    Return:
	/* Remove any interfaces w/o addresses */
	if (rarpd)
		IF_LIST(ii) {
			if (ii->ii_alist.tqh_first == NULL) {
				TAILQ_REMOVE(&iflist, ii, ii_link);
				free(ii->ii_ifname);
				(void)close(ii->ii_fd);
				free(ii);
			    }
		    } IF_LIST_END(ii) ;

	if (dflag) {
		(void)fprintf(stderr, "Interfaces:\n");
		IF_LIST(ii) {
			(void)fprintf(stderr, "\tinterface %s\n",
			    ii->ii_ifname);
			ADDR_LIST(ii, ai) {
				(void)fprintf(stderr, "\t\taddr %s",
				     inet_ntoa(ai->ai_ipaddr));
				(void)fprintf(stderr, " mask %s\n",
				     inet_ntoa(ai->ai_netmask));
			} ADDR_LIST_END(ii, ai) ;
		} IF_LIST_END(ii) ;
	}
	
	(void)free(ifaddrs);
}

static void
usage()
{
	static const char *options = "[-dfA] [-t timeout]";

	if (rarpd) {
		(void)fprintf(stderr, "usage: %s %s interface [address]\n",
		     progname, options);
		(void)fprintf(stderr, "       %s -a %s interface\n",
		     progname, options);
		(void)fprintf(stderr, "       %s -a %s\n",
		     progname, options);
	} else {
		(void)fprintf(stderr, "usage: %s %s interface\n",
		     progname, options);
	}
	
	exit(1);
}

static void
handler(sig)
	int sig;
{
    	struct if_info *ii;
	struct addr_info *ai;

	if (rarpd && (sig == SIGALRM || sig == SIGHUP)) {
		rescan++;
	} else {
		debug("exiting on sig%s", sys_signame[sig]);

		IF_LIST(ii) {
			ADDR_LIST(ii, ai) {
				TAILQ_REMOVE(&ii->ii_alist, ai, ai_link);
				free(ai);
			} ADDR_LIST_END(ii, ai) ;
			TAILQ_REMOVE(&iflist, ii, ii_link);
			free(ii->ii_ifname);
			(void)close(ii->ii_fd);
			free(ii);
		} IF_LIST_END(ii) ;

		exit(1);
	}
}

static int
bpf_open()
{
	int n;
	int fd;
	char device[sizeof "/dev/bpf000"];

	n = 0;
	/* Go through all the minors and find one that isn't in use. */
	do {
		(void)snprintf(device, sizeof device, "/dev/bpf%d", n++);
		fd = open(device, O_RDWR);
	} while (fd < 0 && errno == EBUSY);

	if (fd < 0)
		serr(LOG_ERR, "%s: %s", device, strerror(errno));
	return fd;
}
/*
 * Open a BPF file and attach it to the interface named 'device'.
 * Set immediate mode, and set a filter that accepts only RARP requests.
 */

#define	UNDEF	0

static struct bpf_insn insns_ether[] = {
#define	EINSN_ETYPE	0
	BPF_STMT(BPF_LD | BPF_H | BPF_ABS, UNDEF),
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ETHERTYPE_REVARP, 0, 3),
#define	EINSN_ATYPE	2		/* ar_op field */
	BPF_STMT(BPF_LD | BPF_H | BPF_ABS, UNDEF),
#define	EINSN_AVALUE	3
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, UNDEF, 0, 1),
#define	EINSN_LEN	4		/* Header length + struct ether_arp */
	BPF_STMT(BPF_RET | BPF_K, UNDEF),
	BPF_STMT(BPF_RET | BPF_K, 0),
};

static int
rarp_open(ifap)
	struct ifaddrs *ifap;
{
	struct if_data *ifd;
	struct ifreq ifr;
	struct arphdr *ah;
	struct bpf_program filter;
	int fd;
	int immediate;
	u_int dlt;

	ifd = ifap->ifa_data;

	fd = bpf_open();

	/* Set immediate mode so packets are processed as they arrive. */
	immediate = 1;
	if (ioctl(fd, BIOCIMMEDIATE, &immediate) < 0)
		serr(LOG_ERR, "BIOCIMMEDIATE: %s", strerror(errno));
	(void)strncpy(ifr.ifr_name, ifap->ifa_name, sizeof ifr.ifr_name);
	if (ioctl(fd, BIOCSETIF, &ifr) < 0)
		serr(LOG_ERR, "BIOCSETIF: %s", strerror(errno));
	/* Check that the data link layer is an Ethernet; this code won't work
	 * with anything else. */
	if (ioctl(fd, BIOCGDLT, &dlt) < 0)
		serr(LOG_ERR, "BIOCGDLT: %s", strerror(errno));

	/* Adjust the program for the specific link type */
	ah = NULL;
	switch (dlt) {
		struct ether_header *eh;
		struct ether_8021q_header *veh;

	case DLT_EN10MB:
		eh = NULL;
		insns_ether[EINSN_ETYPE].k =
		    (u_char *)&eh->ether_type - (u_char *)eh;
		insns_ether[EINSN_ATYPE].k = ifd->ifi_hdrlen + 
		    (u_char *)&ah->ar_op - (u_char *)ah;
		insns_ether[EINSN_LEN].k = 
		    ifd->ifi_hdrlen + sizeof(struct ether_arp);

	Ether:
		insns_ether[EINSN_AVALUE].k = rarpd ? ARPOP_REVREQUEST : ARPOP_REVREPLY;

		filter.bf_len = sizeof insns_ether / sizeof(insns_ether[0]);
		filter.bf_insns = insns_ether;
		break;

	case DLT_8021Q:
		veh = NULL;
		insns_ether[EINSN_ETYPE].k = 
		    (u_char *)&veh->evlq_proto - (u_char *)veh;
		insns_ether[EINSN_ATYPE].k = ifd->ifi_hdrlen + 
		    (u_char *)&ah->ar_op - (u_char *)ah;
		insns_ether[EINSN_LEN].k = 
		    ifd->ifi_hdrlen + sizeof(struct ether_arp);
		goto Ether;

	default:
		serr(LOG_ERR, "%s is not a supported interface type",
		    ifap->ifa_name);
	}
	/* Set filter program. */
	if (ioctl(fd, BIOCSETF, &filter) < 0)
		serr(LOG_ERR, "BIOCSETF: %s", strerror(errno));
	return fd;
}
/*
 * Perform various sanity checks on the RARP request packet.  Return
 * false on failure and log the reason.
 */
static int
rarp_check(ii, pkt, len)
	struct if_info *ii;
	void *pkt;
	int len;
{
	struct ether_arp *ap;
	int elen;
	u_int16_t etype;
	u_char *shost;

	debug("got a packet on %s", ii->ii_ifname);

	switch (ii->ii_type) {
		struct ether_header *ep;
		struct ether_8021q_header *vep;

	case IFT_ETHER:
		ep = pkt;
		etype = ntohs(ep->ether_type);
		shost = ep->ether_shost;
		elen = sizeof(*ep);
		ap = (struct ether_arp *)(ep + 1);
		break;

	case IFT_L2VLAN:
		vep = pkt;
		shost = vep->evlq_shost;
		etype = ntohs(vep->evlq_proto);
		elen = sizeof(*vep);
		ap = (struct ether_arp *)(vep + 1);
		break;
	}

	if ((size_t) len < elen + sizeof(*ap)) {
		serr(LOG_INFO, "truncated request");
		return 0;
	}

	if (dflag) {
		char buf[LINE_MAX];

		print_arp(ii, buf, sizeof buf, pkt);
		debug("RECV %s", buf);
	}
	
	/* XXX This test might be better off broken out... */
	if (etype != ETHERTYPE_REVARP ||
	    ntohs(ap->arp_hrd) != ARPHRD_ETHER ||
	    ntohs(ap->arp_op) != (rarpd ? ARPOP_REVREQUEST : ARPOP_REVREPLY) ||
	    ntohs(ap->arp_pro) != ETHERTYPE_IP ||
	    ap->arp_hln != ETHER_ADDR_LEN || ap->arp_pln != 4) {
		serr(LOG_INFO, "request fails sanity check");
		return 0;
	}
	if (memcmp(shost, ap->arp_sha, ETHER_ADDR_LEN) != 0) {
		serr(LOG_WARNING, "ether/arp sender address mismatch");
		return 0;
	}
	if (rarpd && memcmp(ap->arp_sha, ap->arp_tha, ETHER_ADDR_LEN) != 0) {
		serr(LOG_WARNING, "ether/arp target address mismatch");
		return 0;
	}
	return 1;
}

/*
 * Loop indefinitely listening for RARP requests on the
 * interfaces in 'iflist'.
 */
static void
rarp_loop()
{
	struct if_info *ii;
	fd_set fds, listeners;
	int cc, fd;
	int bufsize, maxfd;
	u_char *buf, *bp, *ep;

	maxfd = 0;
	while ((ii = iflist.tqh_first) == NULL) {
		debug("no interfaces");
		sigpause(0);
		if (rescan) {
			rescan = 0;
			interface_scan();
		}
	}
	if (ioctl(ii->ii_fd, BIOCGBLEN, &bufsize) < 0)
		serr(LOG_ERR, "BIOCGBLEN: %s", strerror(errno));
	buf = (u_char *)malloc((unsigned)bufsize);
	if (buf == NULL)
		serr(LOG_ERR, "malloc: %s", strerror(errno));
	/*
         * Find the highest numbered file descriptor for select().
         * Initialize the set of descriptors to listen to.
         */
	FD_ZERO(&fds);
	IF_LIST(ii) {
		FD_SET(ii->ii_fd, &fds);
		if (ii->ii_fd > maxfd)
			maxfd = ii->ii_fd;
	} IF_LIST_END(ii) ;
	while (1) {
		listeners = fds;
		cc = select(maxfd + 1, &listeners, NULL, NULL, NULL);
		if (cc < 0) {
			if (errno == EINTR) {
				if (rescan) {
					rescan = 0;
					interface_scan();
				}
				continue;
			}
			serr(LOG_ERR, "select: %s", strerror(errno));
		}
		IF_LIST(ii) {
			fd = ii->ii_fd;
			if (!FD_ISSET(fd, &listeners))
				continue;
			while ((cc = read(fd, (char *)buf, bufsize)) < 0) {
				if (errno == EINTR)
					continue;
				serr(LOG_ERR, "read: %s", strerror(errno));
			}
			
			/* Loop through the packet(s) */
#define bhp ((struct bpf_hdr *)bp)
			bp = buf;
			ep = bp + cc;
			while (bp < ep) {
				int caplen, hdrlen;

				caplen = bhp->bh_caplen;
				hdrlen = bhp->bh_hdrlen;
				if (rarp_check(ii, bp + hdrlen, caplen)) {
					if (rarpd)
						rarp_process(ii, bp + hdrlen);
					else
						rarp_response(ii, bp + hdrlen);
				}
				bp += BPF_WORDALIGN(hdrlen + caplen);
			}
		} IF_LIST_END(ii) ;
	}
}

/*
 * True if this server can boot the host whose IP address is 'addr'.
 * This check is made by looking in the tftp directory for the
 * SUN-style boot file.
 */
int
rarp_bootable(addr)
	struct in_addr addr;
{
	struct dirent *dent;
	int len;
	char ipname[9];

	/* if unrestricted, always respond */
	if (Aflag)
		return 1;

	len = snprintf(ipname, sizeof ipname, "%08x", ntohl(addr.s_addr));

	rewinddir(dir_handle);
	while ((dent = readdir(dir_handle)))
		if (strncasecmp(dent->d_name, ipname, len) == 0)
			return 1;

	debug("%s (%s) is not bootable", inet_ntoa(addr), ipname);

	return 0;
}

/*
 * Answer the RARP request in 'pkt', on the interface 'ii'.  'pkt' has
 * already been checked for validity.  The reply is overlaid on the request.
 */
void
rarp_process(ii, pkt)
	struct if_info *ii;
	void *pkt;
{
	struct addr_info *ai;
	struct hostent *hp;
	struct in_addr in;
	u_long network;
	char **ap;
	u_char *shost;
	char ename[2048];	/* ether_ntohost() does not take a length */

	switch (ii->ii_type) {
	case IFT_ETHER:
		shost = ((struct ether_header *)pkt)->ether_shost;
		break;

	case IFT_L2VLAN:
		shost = ((struct ether_8021q_header *)pkt)->evlq_shost;
		break;
	}

	if (ether_ntohost(ename, shost) != 0 ||
	    (hp = gethostbyname(ename)) == 0) {
		debug("ntohost() or hostbyname() failed");
		return;
	}
	debug("ethers name is: %s", ename);

	/* Choose correct address from list. */
	if (hp->h_addrtype != AF_INET) {
		serr(LOG_INFO, "can not handle non IP addresses");
		return;
	}
	
	/* Search for address on this interface */
	ADDR_LIST(ii, ai) {
		network = ai->ai_ipaddr.s_addr & ai->ai_netmask.s_addr;

		for (ap = hp->h_addr_list; *ap != NULL; ap++) {
			memcpy(&in.s_addr, *ap, hp->h_length);
			
			if ((in.s_addr & ai->ai_netmask.s_addr) == network) {
				
				goto Reply;
			}
		}
	} ADDR_LIST_END(ii, ai) ;

	serr(LOG_INFO, "can not find %s on interface %s",
	    ename, ii->ii_ifname);
	return;

    Reply:
	if (rarp_bootable(in))
		rarp_reply(ii, ai, pkt, in);
}

/*
 * Poke the kernel arp tables with the ethernet/ip address combinataion
 * given.  When processing a reply, we must do this so that the booting
 * host (i.e. the guy running rarpd), won't try to ARP for the hardware
 * address of the guy being booted (he cannot answer the ARP).
 */
void
update_arptab(ep, ipaddr)
	u_char *ep;
	struct in_addr ipaddr;
{
	FILE *pipe;
	size_t len;
	int wstatus;
	char *cp;
	char buf[LINE_MAX];

	(void)snprintf(buf, sizeof(buf),
	    "arp -sn %s %02x:%02x:%02x:%02x:%02x:%02x temp", inet_ntoa(ipaddr), 
	    ep[0], ep[1], ep[2], ep[3], ep[4], ep[5]);

	if ((pipe = popen(buf, "r")) == NULL) {
		serr(LOG_WARNING, "unable to invoke '%s': %s", buf,
		    strerror(errno));
		return;
	}

	while ((cp = fgetln(pipe, &len)) != NULL) {
		if (cp[len - 1] == '\n')
			len--;
		cp[len] = 0;
		serr(LOG_WARNING, "arp: %s", cp);
	}
	wstatus = pclose(pipe);

	if (WIFSIGNALED(wstatus))
		serr(LOG_WARNING, "'%s' died with sig%s", buf, 
		    sys_signame[WTERMSIG(wstatus)]);
	else if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) != 0)
		serr(LOG_WARNING, "'%s' exited with %d", buf, 
		    WEXITSTATUS(wstatus));

}
/*
 * Build a reverse ARP packet and sent it out on the interface.
 * 'ep' points to a valid ARPOP_REVREQUEST.  The ARPOP_REVREPLY is built
 * on top of the request, then written to the network.
 *
 * RFC 903 defines the ether_arp fields as follows.  The following comments
 * are taken (more or less) straight from this document.
 *
 * ARPOP_REVREQUEST
 *
 * arp_sha is the hardware address of the sender of the packet.
 * arp_spa is undefined.
 * arp_tha is the 'target' hardware address.
 *   In the case where the sender wishes to determine his own
 *   protocol address, this, like arp_sha, will be the hardware
 *   address of the sender.
 * arp_tpa is undefined.
 *
 * ARPOP_REVREPLY
 *
 * arp_sha is the hardware address of the responder (the sender of the
 *   reply packet).
 * arp_spa is the protocol address of the responder (see the note below).
 * arp_tha is the hardware address of the target, and should be the same as
 *   that which was given in the request.
 * arp_tpa is the protocol address of the target, that is, the desired address.
 *
 * Note that the requirement that arp_spa be filled in with the responder's
 * protocol is purely for convenience.  For instance, if a system were to use
 * both ARP and RARP, then the inclusion of the valid protocol-hardware
 * address pair (arp_spa, arp_sha) may eliminate the need for a subsequent
 * ARP request.
 */
static void
rarp_reply(ii, ai, pkt, ipaddr)
	struct if_info *ii;
	struct addr_info *ai;
	void *pkt;
	struct in_addr ipaddr;
{
	struct ether_arp *ap;
	int n;
	int len;

	debug("sending rarp reply");

	/* Build the rarp reply by modifying the rarp request in place. */
	switch (ii->ii_type) {
		struct ether_header *ep;
		struct ether_8021q_header *vep;

	case IFT_ETHER:
		ep = pkt;
		ap = (struct ether_arp *)(ep + 1);
		ep->ether_type = htons(ETHERTYPE_REVARP);
		(void)memcpy(ep->ether_dhost, ap->arp_sha, ETHER_ADDR_LEN);
		(void)memcpy(ep->ether_shost, ii->ii_eaddr, ETHER_ADDR_LEN);
		len = sizeof(*ep) + sizeof(*ap);
		break;

	case IFT_L2VLAN:
		vep = pkt;
		ap = (struct ether_arp *)(vep + 1);
		vep->evlq_proto = htons(ETHERTYPE_REVARP);
		(void)memcpy(vep->evlq_dhost, ap->arp_sha, ETHER_ADDR_LEN);
		(void)memcpy(vep->evlq_shost, ii->ii_eaddr, ETHER_ADDR_LEN);
		len = sizeof(*vep) + sizeof(*ap);
		break;
	}

	update_arptab((u_char *)&ap->arp_sha, ipaddr);

	ap->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
	ap->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
	ap->arp_op = htons(ARPOP_REVREPLY);

	(void)memcpy(ap->arp_sha, ii->ii_eaddr, ETHER_ADDR_LEN);

	(void)memcpy(ap->arp_tpa, &ipaddr.s_addr, 4);
	/* Target hardware is unchanged. */
	(void)memcpy(ap->arp_spa, &ai->ai_ipaddr, 4);

	n = write(ii->ii_fd, pkt, len);
	if (n != len)
		serr(LOG_INFO, "rarp_reply: only %d of %d bytes written%s%s",
		    n, len, 
		    n == -1 ? ": " : "",
		    n == -1 ? strerror(errno) : "");
	if (dflag) {
		char buf[LINE_MAX];

		print_arp(ii, buf, sizeof buf, pkt);
		debug("SEND %s", buf);
	}
}

/* Send a request for our address */
static void
rarp_request(ii)
	struct if_info *ii;
{
	struct ether_arp *ap;
	u_char buf[1024];
	int len;
	int n;

	switch (ii->ii_type) {
		struct ether_header *ep;
		struct ether_8021q_header *vep;

	case IFT_ETHER:
		ep = (struct ether_header *)buf;
		len = sizeof(*ep) + sizeof(*ap);
		ap = (struct ether_arp *)(ep + 1);

		/* Build the Ethernet header */
		(void)memset(ep->ether_dhost, 0xff, ETHER_ADDR_LEN);
		(void)memcpy(ep->ether_shost, ii->ii_eaddr, ETHER_ADDR_LEN);
		ep->ether_type = htons(ETHERTYPE_REVARP);
		break;

	case IFT_L2VLAN:
		vep = (struct ether_8021q_header *)buf;
		len = sizeof(*vep) + sizeof(*ap);
		ap = (struct ether_arp *)(vep + 1);

		/* Build the 802.1Q header */
		(void)memset(vep->evlq_dhost, 0xff, ETHER_ADDR_LEN);
		(void)memcpy(vep->evlq_shost, ii->ii_eaddr, ETHER_ADDR_LEN);
		vep->evlq_encap_proto = htons(ETHERTYPE_8021Q);
		vep->evlq_tci = htons(ii->ii_tci);
		vep->evlq_proto = htons(ETHERTYPE_REVARP);
		break;
	}

	debug("sending rarp request to %s", ii->ii_ifname);


	/* Build the ARP packet */
	ap->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
	ap->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
	ap->ea_hdr.ar_hln = ETHER_ADDR_LEN;
	ap->ea_hdr.ar_pln = sizeof ap->arp_spa;
	ap->arp_op = htons(ARPOP_REVREQUEST);
	(void)memcpy(ap->arp_sha, ii->ii_eaddr, ETHER_ADDR_LEN);
	(void)memset(ap->arp_spa, 0, sizeof ap->arp_spa);
	(void)memcpy(ap->arp_tha, ii->ii_eaddr, ETHER_ADDR_LEN);
	(void)memset(ap->arp_tpa, 0, sizeof ap->arp_tpa);

	n = write(ii->ii_fd, buf, len);
	if (n != len)
		serr(LOG_INFO, "rarp_request: only %d of %d bytes written%s%s",
		    n, len, 
		    n == -1 ? ": " : "",
		    n == -1 ? strerror(errno) : "");
	if (dflag) {
		char pbuf[LINE_MAX];

		print_arp(ii, pbuf, sizeof pbuf, buf);
		debug("SEND %s", pbuf);
	}
}


/* Process a response */
static void
rarp_response(ii, pkt)
	struct if_info *ii;
	void *pkt;
{
	struct ether_arp *ap;
	struct in_addr in;

	switch (ii->ii_type) {
		struct ether_header *ep;
		struct ether_8021q_header *vep;

	case IFT_ETHER:
		ep = pkt;
		ap = (struct ether_arp *)(ep + 1);
		break;

	case IFT_L2VLAN:
		vep = pkt;
		ap = (struct ether_arp *)(vep + 1);
		break;
	}

	debug("received response");
	if (memcmp(ap->arp_tha, ii->ii_eaddr, ETHER_ADDR_LEN) != 0) {
		debug("response not for me");
		return;
	}

	(void)memcpy(&in.s_addr, ap->arp_tpa, sizeof in.s_addr);
	printf("%s\n", inet_ntoa(in));

	exit(0);
}

static void
print_arp(ii, cp, tlen, pkt)
	struct if_info *ii;
	char *cp;
	int tlen;
	void *pkt;
{
	int len;
	struct in_addr in;
	struct ether_arp *ap;

	switch (ii->ii_type) {
		struct ether_header *ep;
		struct ether_8021q_header *vep;

	case IFT_ETHER:
		ep = pkt;
		len = snprintf(cp, tlen,
		    "%x:%x:%x:%x:%x:%x %x:%x:%x:%x:%x:%x %04x ",
		    ep->ether_dhost[0], ep->ether_dhost[1],
		    ep->ether_dhost[2], ep->ether_dhost[3],
		    ep->ether_dhost[4], ep->ether_dhost[5],
		    ep->ether_shost[0], ep->ether_shost[1],
		    ep->ether_shost[2], ep->ether_shost[3],
		    ep->ether_shost[4], ep->ether_dhost[5],
		    ntohs(ep->ether_type));
		ap = (struct ether_arp *)(ep + 1);
		break;

	case IFT_L2VLAN:
		vep = pkt;
		len = snprintf(cp, tlen,
		    "%x:%x:%x:%x:%x:%x %x:%x:%x:%x:%x:%x %04x %04x ",
		    vep->evlq_dhost[0], vep->evlq_dhost[1],
		    vep->evlq_dhost[2], vep->evlq_dhost[3],
		    vep->evlq_dhost[4], vep->evlq_dhost[5],
		    vep->evlq_shost[0], vep->evlq_shost[1],
		    vep->evlq_shost[2], vep->evlq_shost[3],
		    vep->evlq_shost[4], vep->evlq_dhost[5],
		    ntohs(vep->evlq_tci),
		    ntohs(vep->evlq_proto));
		ap = (struct ether_arp *)(vep + 1);
		break;
	}

	len += snprintf(cp + len, tlen - len,
	    "hrd %u pro %04x hln %u pln %u op %u ",
	    ntohs(ap->ea_hdr.ar_hrd),
	    ntohs(ap->ea_hdr.ar_pro),
	    ap->ea_hdr.ar_hln,
	    ap->ea_hdr.ar_pln,
	    ntohs(ap->arp_op));
	(void)memcpy(&in.s_addr, ap->arp_spa, sizeof in.s_addr);
	len += snprintf(cp + len, tlen - len, "%x:%x:%x:%x:%x:%x %s ",
	    ap->arp_sha[0], ap->arp_sha[1],
	    ap->arp_sha[2], ap->arp_sha[3],
	    ap->arp_sha[4], ap->arp_sha[5],
	    inet_ntoa(in));
	(void)memcpy(&in.s_addr, ap->arp_tpa, sizeof in.s_addr);
	len += snprintf(cp + len, tlen - len, "%x:%x:%x:%x:%x:%x %s ",
	    ap->arp_tha[0], ap->arp_tha[1],
	    ap->arp_tha[2], ap->arp_tha[3],
	    ap->arp_tha[4], ap->arp_tha[5],
	    inet_ntoa(in));
}

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#if __STDC__
debug(const char *fmt,...)
#else
debug(fmt, va_alist)
	char   *fmt;
va_dcl
#endif
{
	va_list ap;

	if (dflag) {
#if __STDC__
		va_start(ap, fmt);
#else
		va_start(ap);
#endif
		(void)fprintf(stderr, "%s: ", progname);
		(void)vfprintf(stderr, fmt, ap);
		va_end(ap);
		(void)fprintf(stderr, "\n");
	}
}

static void
#ifdef	__STDC__
serr(int prio, const char *fmt, ...)
#else
serr(error, fmt, va_alist)
	int prio;
	const char *fmt;
va_dcl
#endif
{
	va_list ap;

#ifdef	__STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	if (dflag || !rarpd) {
		(void)fprintf(stderr, "%s: ", progname);
		(void)vfprintf(stderr, fmt, ap);
		va_end(ap);
		(void)fprintf(stderr, "\n");
	} else {
		vsyslog(prio, fmt, ap);
	}
	if (prio <= LOG_ERR)
		exit(1);
}

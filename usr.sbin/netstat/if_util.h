/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI if_util.h,v 2.1 1997/10/31 04:13:38 jch Exp
 */

/* Structure we use when saving info about an address */
struct if_entry {
	struct if_msghdr *ife_msghdr;
	struct sockaddr_dl *ife_addr;
};

/* Short cuts to interface addresses */
#define	ifaddr(p)	(p)->rti_info[RTAX_IFA]
#define	netmask(p)	(p)->rti_info[RTAX_NETMASK]
#define	dstaddr(p)	(p)->rti_info[RTAX_BRD]

/* Routines we provide */
extern struct if_entry *if_byindex __P((struct if_msghdr *, int));
extern void if_loop __P((kvm_t *, char *, int,
	    void (*linkfn) (struct if_msghdr *, struct sockaddr_dl *, char *),
	    void (*addrfn) (struct if_msghdr *, struct sockaddr_dl *, char *,
		struct ifa_msghdr *, struct rt_addrinfo *)));
extern void if_xaddrs __P((void *, int, struct rt_addrinfo *));

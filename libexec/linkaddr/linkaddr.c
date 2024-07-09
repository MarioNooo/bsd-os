/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI linkaddr.c,v 1.1 1996/08/31 00:05:21 sanders Exp
 */

/*
 * linkaddr -- prints link addresses given IP Addresses
 *
 * It attempts to match the subnet for the given IP Address on a local
 * interface.  This program intended to be used by the virtual host
 * configuration script that sets up proxy arps for local IP Addresses
 * (other IP Addresses must use routing to get packets to the local machine).
 *
 * Output format is IP Address followed by Link Address (if known, else blank)
 *
 * Example:
 *     % /usr/libexec/linkaddr 205.230.224.250 10.99.99.99
 *     205.230.224.250 00:a0:24:12:da:13
 *     10.99.99.99
 *    
 */

#include <sys/param.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>

#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define	SA(x)	((struct sockaddr_in *)(x))
#define	SAIN(x)	SA(x)->sin_addr.s_addr

static	void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct ifaddrs *ifaddrs, *ifap;
	struct ifaddrs *ifa_save;
	struct sockaddr_dl *sdl, *sdl_save;
	struct in_addr addr;
	int ch;
	u_char *cp, *ep;

	while ((ch = getopt(argc, argv, "d")) != -1)
		switch (ch) {
		case 'd':
			break;
			
		case '?':
		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();
	
	if (getifaddrs(&ifaddrs) < 0)
		err(1, "getifaddrs");

	for (; argc != 0; argc--, argv++) {

		if (inet_aton(*argv, &addr) == 0)
			warnx("invalid address: %s", *argv);
		
		sdl = sdl_save = NULL;
		ifa_save = NULL;
		for (ifap = ifaddrs; ifap != NULL; ifap = ifap->ifa_next) {
			if (ifap->ifa_addr->sa_family == AF_LINK) {
				sdl = (struct sockaddr_dl *)ifap->ifa_addr;
				continue;
			} else if (ifap->ifa_addr->sa_family != AF_INET ||
			    (ifap->ifa_flags & IFF_BROADCAST) == 0)
				continue;

			if (((SAIN(ifap->ifa_addr) ^ addr.s_addr) &
			    SAIN(ifap->ifa_netmask)) == 0 &&
			    (ifa_save == NULL ||
			    ntohl(SAIN(ifap->ifa_netmask)) >
			    ntohl(SAIN(ifa_save->ifa_netmask)))) {
				ifa_save = ifap;
				sdl_save = sdl;
			}
		}

		printf("%s ", inet_ntoa(addr));
		if (sdl_save != NULL)
			for (cp = (u_char *)LLADDR(sdl_save),
			    ep = cp + sdl_save->sdl_alen;
			    cp < ep; cp++)
				printf("%s%02x",
				    (caddr_t)cp == LLADDR(sdl_save)
				    ? "" : ":", *cp & 0xff);
		printf("\n");
	}

	return 0;
}

static void
usage()
{
	fprintf(stderr, "Usage: linkaddr addr1 addr2 addr3 ... addrN\n");
	exit(1);
}

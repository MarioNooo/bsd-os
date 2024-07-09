/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI pppstat.c,v 1.2 1996/09/29 15:46:15 bostic Exp
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ppp.h>
#include <net/if_pppioctl.h>
#include <net/if_types.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char **argv)
{
	struct ifreq ifr;
	struct ppp_ioctl *pio = ifr_pppioctl(&ifr);
	char *space;
	int skt, verbose;

	verbose = 0;
	while ((skt = getopt(argc, argv, "v")) != EOF)
		switch (skt) {
		case 'v':
			verbose = 1;
			break;
		default: usage:
			fprintf(stderr,
			    "usage: pppstat [-v] interface [...]\n");
			exit(1);
		}

	if (optind >= argc)
		goto usage;

	if ((skt = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "socket(AF_INET, SOCK_DGRAM)");

	for (; argv[optind]; ++optind) {
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, argv[optind], sizeof(ifr.ifr_name));
		if (ioctl(skt, PPPIOCGSTATE, (caddr_t) &ifr) < 0) {
			warn(argv[optind]);
			continue;
		}
		printf("%s:	MTU: %d, idle timeout: %d seconds\n",
		    argv[optind], pio->ppp_mtu, pio->ppp_idletime);
		printf("	txmap: 0x%08lx, rxmap: 0x%08lx\n",
		    pio->ppp_txcmap, pio->ppp_rxcmap);
		printf("	");
		space = "";
		if (pio->ppp_flags & PPP_PFC) {
			printf("%spfc", space);
			space = ", ";
		} else if (verbose) {
			printf("%s-pfc", space);
			space = ", ";
		}
		if (pio->ppp_flags & PPP_ACFC) {
			printf("%sacfc", space);
			space = ", ";
		} else if (verbose)
			printf("%s-acfc", space);
		if (pio->ppp_flags & PPP_TCPC) {
			printf("%stcpc", space);
			space = ", ";
		} else if (verbose)
			printf("%s-tcpc", space);
		if (pio->ppp_flags & PPP_FTEL) {
			printf("%sftel", space);
			space = ", ";
		} else if (verbose)
			printf("%s-ftel", space);
		if (pio->ppp_flags & PPP_IPOKAY) {
			printf("%sipokay", space);
			space = ", ";
		} else if (verbose)
			printf("%s-ipokay", space);
		if (pio->ppp_flags & PPP_IPREJECT) {
			printf("%sipreject", space);
			space = ", ";
		} else if (verbose)
			printf("%s-ipreject", space);
		printf("\n");
	}
	exit (0);
}

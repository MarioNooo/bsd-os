/*-
 *  Copyright (c) 1999 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwrewrite.c,v 1.2 2000/05/02 19:08:39 prb Exp
 */
#define	IPFW
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/bpf.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ipfw.h>

#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ipfw.h"

int
main(int argc, char **argv)
{
	ipfw_filter_t ipfw;
	int c, n;
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
		IPFW_CALL, IPFWCTL_SYSCTL, IPFW_REWRITE };

	memset(&ipfw, 0, sizeof(ipfw));
	ipfw.type = IPFW_REWRITE;
	ipfw.hlength = sizeof(ipfw);
	ipfw.length = 0;
	strncpy(ipfw.tag, "rewrite", IPFW_TAGLEN);

	while ((c = getopt(argc, argv, "T:")) != EOF) {
		switch (c) {
		case 'T':
			strncpy(ipfw.tag, optarg, IPFW_TAGLEN);
			break;
		default: usage:
			fprintf(stderr, "usage: ipfwrewrite [-T tag]\n");
			exit(1);
		}
	}

	if (optind != argc)
		goto usage;

	if (ipfw.serial == 0) {
		n = sizeof(ipfw.serial);
		mib[5] = IPFWCTL_SERIAL;
		if (sysctl(mib, 6, &ipfw.serial, &n, NULL, NULL))
			err(1, "getting serial number");
	}
	mib[5] = IPFWCTL_PUSH;
	if (sysctl(mib, 6, NULL, NULL, &ipfw, sizeof(ipfw)))
		err(1, "pushing rewrite filter");
	printf("Installed rewrite filter as serial #%d\n",
	    ipfw.serial);
	exit (0);
}

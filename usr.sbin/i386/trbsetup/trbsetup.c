/*-
 * Copyright (c) 1998 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI trbsetup.c,v 1.1 1998/09/01 17:39:57 geertj Exp
 *
 * Code based on tesetup.c
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <i386/pci/trbioctl.h>

#include <time.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EMSG sys_errlist[errno]

#define USAGE "Usage: %s ifname\n"

extern	u_short trb_ucode[];

void usage(char *const name);

int
main(int ac, char **av)
{
	int c;
	char *const name = av[0];
	struct ifreq ifr;
	int fd;

	if (ac != 2)
		usage(name);

	strncpy(ifr.ifr_name, av[1], IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = 0;
	ifr.ifr_data = (caddr_t)&trb_ucode[0];

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr,"Can't create socket: %s\n", EMSG);
		return (1);
	}
	if (ioctl(fd, TRBIOCUCODE, (caddr_t)&ifr)) {
		fprintf(stderr,"Microcode load failed: %s\n", EMSG);
		return (1);
	}
	return (0);
}

/*
 * Print usage and exit
 */
void
usage(char *const name)
{
	fprintf(stderr, USAGE, name, name, name, name);
	exit(1);
}

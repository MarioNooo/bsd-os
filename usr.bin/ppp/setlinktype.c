/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI setlinktype.c,v 2.1 1996/11/13 23:54:46 prb Exp
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>

#include <string.h>
#include <unistd.h>

int
setlinktype(char *name, int type)
{
	int r, rs;
	struct sockaddr_dl *dl;
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));

	dl = (struct sockaddr_dl *)&ifr.ifr_addr;

	if ((rs = socket(AF_LINK, SOCK_RAW, 0)) < 0)
		return (-1);

	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));

	dl->sdl_family = AF_LINK;
	dl->sdl_type = type;
	dl->sdl_len = sizeof(*dl) - sizeof(dl->sdl_data);

	r = ioctl(rs, SIOCSIFADDR, &ifr);
	close(rs);
	return(r);
}

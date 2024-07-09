/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_getinfo.c,v 2.1 1995/02/03 15:19:53 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

/*
 * NOT YET	do something about options
 * 2DO		How to set tsdu for SOCK_DGRAM
 */

int
t__getinfo(tsp, ip)
	ts_t *tsp;
	info_t *ip;
{

	ip->addr = sizeof (struct sockaddr);
	ip->options = T_INVALID;
	ip->tsdu = T_NULL;
	ip->etsdu = T_INVALID;
	ip->connect = T_INVALID;
	ip->discon = T_INVALID;
	ip->servtype = tsp->ts_protocol == SOCK_DGRAM ? T_CLTS : T_COTS_ORD;

	return (0);
}
int
t_getinfo(fd, i)
	int fd;
	info_t *i;
{
	ts_t *tsp;

	DBG_ENTER(("t_getinfo", phex, &fd, 0));


	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_getinfo TBADF", -1, 0));
		return -1;
	}

	if (i != NULL)
		t__getinfo(tsp, i);

	DBG_EXIT(("t_getinfo", 0, pt_info, i, 0));
	return (0);
}

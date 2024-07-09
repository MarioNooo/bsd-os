/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_optmgmt.c,v 2.1 1995/02/03 15:20:15 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

/*
 * NOT YET	leave this until it seems to be running
 * NOT YET	If won't get off the ground will have to do this now
 */

int
t_optmgmt(fd, rip, rop)
	int fd;
	t_optmgmt_t *rip;
	t_optmgmt_t *rop;
{
	int res;
	ts_t *tsp;

	DBG_ENTER(("t_optmgmt", phex, &fd, pt_optmgmt, rip, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_optmgmt TBADF", -1, 0));
		return -1;
	}

	t_errno = TNOTSUPPORT;
	DBG_EXIT(("t_optmgmt TNOTSUPPORT", -1, 0));
	return (-1);
}

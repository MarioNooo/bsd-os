/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_sync.c,v 2.1 1995/02/03 15:21:01 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

/*
 * NOT NOW		Put this one off
 */

int
t_sync(fd)
	int fd;
{
	ts_t *tsp;
	int res;

	DBG_ENTER(("t_sync", phex, &fd, 0));

	/*
	 * following is not right
	 * app could have dupped
	 */
	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_sync TBADF", -1, 0));
		return (-1);
	}

	t_errno = TNOTSUPPORT;
	DBG_EXIT(("t_sync TBADF", -1, 0));
	return (-1);
}

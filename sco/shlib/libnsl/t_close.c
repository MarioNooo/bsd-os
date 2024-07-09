/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_close.c,v 2.1 1995/02/03 15:19:33 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

/*
 * NOT YET	Leave closed descriptor around and have open
 * NOT YET	check before allocating a new one.
 */

int 
t_close(fd)
	int fd;
{
	ts_t *tsp, *tsp1;

	DBG_ENTER(("t_close", phex, &fd, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_close TBADF", -1, 0));
		return -1;
	}

	if (ts_list == tsp)
		ts_list = tsp->ts_next;
	else {
		for (tsp1 = ts_list; tsp1->ts_next != tsp; tsp1 = tsp1->ts_next)
			continue;
		tsp1->ts_next = tsp->ts_next;
	}
	memfree(tsp);
	close (fd);
	DBG_EXIT(("t_close", 0, 0));
	return (0);
}

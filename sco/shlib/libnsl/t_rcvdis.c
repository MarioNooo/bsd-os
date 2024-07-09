/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_rcvdis.c,v 2.1 1995/02/03 15:20:26 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

/*
 *
 */

int
t_rcvdis(fd, dp)
	int fd;
	t_discon_t *dp;
{
	struct sockaddr s;
	int res;
	ts_t *tsp;

	DBG_ENTER(("t_rcvdis", phex, &fd, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_rcvdis TBADF", -1, 0));
		return -1;
	}

	if (tsp->ts_state == T_IDLE  ||
	    tsp->ts_state == T_UNBND) {
		t_errno = TOUTSTATE;
		DBG_EXIT(("t_rcvdis TOUTSTATE", -1, 0));
		return (-1);
	}
#if 0
	t_errno = TNODIS;
	DBG_EXIT(("t_rcvdis TNODIS", -1, 0));
	return (-1);
#endif
	DBG_EXIT(("t_rcvdis", 0, 0));
	return (0);
}

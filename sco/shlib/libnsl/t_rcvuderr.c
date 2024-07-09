/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_rcvuderr.c,v 2.1 1995/02/03 15:20:37 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

int
t_rcvuderr(fd, uderr)
	int fd;
	t_uderr_t *uderr;
{
	ts_t *tsp;

	DBG_ENTER(("t_rcvuderr", phex, &fd, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_rcvuderr TBADF", -1, 0));
		return -1;
	}

	t_errno = TNOTSUPPORT;
	DBG_EXIT(("t_rcvuderr TNOTSUPPORT", -1, 0));
	return (-1);
}

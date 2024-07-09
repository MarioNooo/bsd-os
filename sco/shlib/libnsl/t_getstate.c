/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_getstate.c,v 2.1 1995/02/03 15:19:57 polk Exp
 */

#include "nsl_compat.h"

int
t_getstate(fd)
	int fd;
{
	ts_t *tsp;

	if ((tsp = ts_get(fd)) == NULL)
		return (-1);

	return (tsp->ts_state);
}

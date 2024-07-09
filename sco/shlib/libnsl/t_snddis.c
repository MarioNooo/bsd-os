/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_snddis.c,v 2.1 1995/02/03 15:20:48 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

/*
 * 2DO	should this be supported
 */


int
t_snddis(fd, cp)
	int fd;
	t_call_t *cp;
{
	int res;
	ts_t *tsp;

	DBG_ENTER(("t_snddis", phex, &fd, pt_call, cp, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_snddis TBADF", -1, 0));
		return -1;
	}

	t_errno = TNOTSUPPORT;
	DBG_EXIT(("t_snddis TNOTSUPPORT", -1, 0));
	return (-1);
}

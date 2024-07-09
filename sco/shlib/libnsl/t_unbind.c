/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_unbind.c,v 2.1 1995/02/03 15:21:06 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

int
t_unbind(fd)
	int fd;
{
	ts_t *tsp;

	DBG_ENTER(("t_unbind", phex, &fd, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_unbind TBADF", -1, 0));
		return -1;
	}

	if (shutdown(fd, 2)) {
		tli_maperrno();
		t_errno = TSYSERR;
		DBG_EXIT(("t_unbind shutdown: TSYSERR", -1, 0));
		return (-1);
	}
	DBG_EXIT(("t_unbind", 0, 0));
	return (0);
}

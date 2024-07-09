/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_rcvrel.c,v 2.1 1995/02/03 15:20:30 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

int
t_rcvrel(fd)
	int fd;
{
	ts_t *tsp;

	DBG_ENTER(("t_rcvrel", phex, &fd, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_rcvrel TBADF", -1, 0));
		return (-1);
	}

	if (tsp->ts_state != T_DATAXFER &&
	    tsp->ts_state != T_OUREL) {
		t_errno = TOUTSTATE;
		DBG_EXIT(("t_rcvrel TOUTSTATE", -1, 0));
		return (-1);
	}

	if (!tsp->ts_eof) {
		t_errno = TNOREL;
		DBG_EXIT(("t_rcvrel TNOREL", -1, 0));
		return (-1);
	}
		
	if (shutdown(fd, 0)) {
		t_errno = TSYSERR;
		tli_maperrno();
		DBG_EXIT(("t_rcvrel shutdown:", -1, 0));
		return (-1);
	}
	if (tsp->ts_state == T_DATAXFER)
		tsp->ts_state = T_INREL;
	else
		tsp->ts_state = T_UNBND;		/* 2DO verify */

	DBG_EXIT(("t_rcvrel", 0, 0));
	return (0);
}

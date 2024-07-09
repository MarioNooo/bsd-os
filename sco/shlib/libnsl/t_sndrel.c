/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_sndrel.c,v 2.1 1995/02/03 15:20:53 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

/*
 * 2DO recheck
 */

/*
 * 2DO put in flag so we know this has happened 
 */

int
t_sndrel(fd)
	int fd;
{
	ts_t *tsp;
	int res;

	DBG_ENTER(("t_sndrel", phex, &fd, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_sndrel TBADF", -1, 0));
		return (-1);
	}

	if (tsp->ts_state != T_DATAXFER &&
	    tsp->ts_state != T_INREL) {
		t_errno = TOUTSTATE;
		DBG_EXIT(("t_sndrel TOUTSTATE", -1, 0));
		return (-1);
	}

	if (shutdown(fd, 1)) {
		t_errno = TSYSERR;
		tli_maperrno();
		DBG_EXIT(("t_sndrel shutdown:", -1, 0));
		return (-1);
	}

	if (tsp->ts_state == T_DATAXFER)
		tsp->ts_state = T_OUREL;
	else
		tsp->ts_state = T_UNBND;		/* 2DO verify */
		
	DBG_EXIT(("t_sndrel", 0, 0));
	return (0);
}

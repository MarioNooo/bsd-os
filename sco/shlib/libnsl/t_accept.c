/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_accept.c,v 2.1 1995/02/03 15:19:19 polk Exp
 */

/*
 * NOT YET	check for other activity on fd before doing dup2 where
 *		fd  == c->sequence
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

int
t_accept(fd, resfd, cp)
	int fd, resfd;
	call_t *cp;
{
	ts_t *tsp, *tsp1;

	DBG_ENTER(("t_accept", phex, &fd, phex, &resfd, pt_call, cp, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_accept TBADF", -1, 0));
		return -1;
	}

	if (tsp->ts_protocol != SOCK_STREAM) {
		t_errno = TNOTSUPPORT;
		DBG_EXIT(("t_accept TNOTSUPPORT", -1, 0));
		return (-1);
	}

	if (tsp->ts_state != T_INCON) {
		t_errno = TOUTSTATE;
		DBG_EXIT(("t_accept TOUTSTATE", -1, 0));
		return (-1);
	}

	if (dup2(cp->sequence, resfd) < 0) {
		t_errno = TSYSERR;
		tli_maperrno();
		DBG_EXIT(("t_accept dup2 failed", -1, 0));
		return (-1);
	}

	if (cp->sequence != resfd) {
		if ((tsp1 = ts_get(resfd)) == NULL) {
			DBG_EXIT(("t_accept TBADF", -1, 0));
			return -1;
		}
		*tsp1 = *tsp;
		tsp1->ts_state = T_DATAXFER;
		tsp1->ts_fd = resfd;
	} else
		tsp->ts_state = T_DATAXFER;

	DBG_EXIT(("t_accept", 0, 0));
	return (0);
}

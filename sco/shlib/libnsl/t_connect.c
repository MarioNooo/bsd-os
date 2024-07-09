/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_connect.c,v 2.1 1995/02/03 15:19:38 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

int
t_connect(fd, cip, cop)
	int fd;
	call_t *cip, *cop;

{
	struct sockaddr si, so;
	ts_t *tsp;

	DBG_ENTER(("t_connect", phex, &fd, pt_call, cip, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_connect TBADF", -1, 0));
		return -1;
	}

	if (tsp->ts_state != T_IDLE) {
		t_errno = TOUTSTATE;
		DBG_EXIT(("t_connect TOUTSTATE", -1, 0));
		return (-1);
	}

	if (sock_in(&cip->addr, &si, sizeof (si), 1)) {
		DBG_EXIT(("t_connect sock_in:", -1, 0));
		return (-1);
	}

	if (connect(fd, &si, sizeof (si)) == -1) {
		switch (errno) {
		case EADDRNOTAVAIL:
			t_errno = TNOADDR;
			tsp->ts_state = T_OUTCON;
			break;
		case EADDRINUSE:
			t_errno = TADDRBUSY;
			break;
		case EINPROGRESS:	/* 2DO is getname valid */
			t_errno = TNODATA;
		default:
			t_errno = TSYSERR;
			tli_maperrno();
		}
		DBG_EXIT(("t_connect connect: failed", -1, 0));
		return (-1);
	}
	tsp->ts_state = T_DATAXFER;
	if (cop != NULL) {
		int i = sizeof (so);
		if (getsockname(fd, &so, &i)) {
			t_errno = TSYSERR;
			tli_maperrno();
			DBG_EXIT(("t_connect getsockname: failed", -1, 0));
			return (-1);
		}
		cop->addr.len = i;
		if (sock_out(&cop->addr, &so)) {
			DBG_EXIT(("t_connect sock_out", -1, 0));
			return (-1);
		}
	}
	DBG_EXIT(("t_connect", 0, pt_call, cop, 0));
	return (0);
}

/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_rcvconnect.c,v 2.1 1995/02/03 15:20:22 polk Exp
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include "nsl_compat.h"
#include "nsl_debug.h"

int
t_rcvconnect(fd, cp)
	int fd;
	t_call_t *cp;
{
	ts_t *tsp;
	struct sockaddr so;
	fd_set readfds, writefds, exceptfds;
	struct timeval timeval;

	DBG_ENTER(("t_rcvconnect", phex, &fd, pt_call, cp, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_rcvconnect TBADF", -1, 0));
		return (-1);
	}

	if (tsp->ts_state != T_OUTCON) {
		t_errno = TOUTSTATE;
		DBG_EXIT(("t_rcvconnect TOUTSTATE", -1, 0));
		return (-1);
	}

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	FD_SET(fd, &readfds);
	FD_SET(fd, &writefds);
	FD_SET(fd, &exceptfds);
	bzero(&timeval, sizeof (timeval));

	select(fd + 1, &readfds, &writefds, &exceptfds, &timeval); 

	if (!FD_ISSET(fd, &readfds) && !FD_ISSET(fd, &writefds)) {
		t_errno = TNODATA;
		DBG_EXIT(("t_rcvconnect TNODATA", -1, 0));
		return (-1);
	}
	tsp->ts_state = T_DATAXFER;
	if (cp != NULL) {
		int i = sizeof (so);
		if (getsockname(fd, &so, &i)) {
			t_errno = TSYSERR;
			tli_maperrno();
			DBG_EXIT(("t_rcvconnect getsockname: TSYSERR", -1, 0));
			return (-1);
		}
		cp->addr.len = i;
		if (sock_out(&cp->addr, &so)) {
			DBG_EXIT(("t_rcvconnect sock_out:", -1, 0));
			return (-1);
		}
	}
	DBG_EXIT(("t_rcvconnect", pt_call, cp, 0));
	return (0);
}

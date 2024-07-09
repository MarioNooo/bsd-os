/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_look.c,v 2.1 1995/02/03 15:20:05 polk Exp
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include "nsl_compat.h"
#include "nsl_debug.h"


int
t_look(fd)
	int fd;
{
	int res;
	ts_t *tsp;
	fd_set readfds, writefds, exceptfds;
	struct timeval timeval;

	DBG_ENTER(("t_look", phex, &fd, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_look TBADF", -1, 0));
		return (-1);
	}
	if (tsp->ts_event) {
		res = tsp->ts_event;
		tsp->ts_event = 0;
		DBG_EXIT(("t_look event", res, 0));
		return (res);
	}

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	FD_SET(fd, &readfds);
	FD_SET(fd, &writefds);
	FD_SET(fd, &exceptfds);
	bzero(&timeval, sizeof (timeval));

	select(fd + 1, &readfds, &writefds, &exceptfds, &timeval); 

	switch (tsp->ts_state) {
	case T_UNBND:
		DBG_EXIT(("t_look", 0, 0));
		return (0);

	case T_IDLE:
		if (tsp->ts_protocol == SOCK_STREAM) {
			if (FD_ISSET(fd, &readfds)) {
				DBG_EXIT(("t_look", T_LISTEN, 0));
				return (T_LISTEN);
			}
			DBG_EXIT(("t_look", 0, 0));
			return (0);
		}
		if (FD_ISSET(fd, &readfds)) {
			DBG_EXIT(("t_look", T_DATA, 0));
			return (T_DATA);
		}
		DBG_EXIT(("t_look", 0, 0));
		return (0);

	/*
	 *	Really need to go over these some more
	 *	2DO only check readfds or writefds 
	 *	2DO T_INCON may not be same as T_OUTCON
	 */
	case T_INCON:
		if (FD_ISSET(fd, &readfds)) {
			DBG_EXIT(("t_look", T_CONNECT, 0));
			return (T_CONNECT);
		}
	case T_OUTCON:
		if (FD_ISSET(fd, &readfds)) {
			DBG_EXIT(("t_look", T_CONNECT, 0));
			return (T_CONNECT);
		}

	case T_DATAXFER:
	case T_OUREL:
		if (FD_ISSET(fd, &readfds)) {
			DBG_EXIT(("t_look", T_DATA, 0));
			return (T_DATA);
		}

	case T_INREL:
		DBG_EXIT(("t_look", 0, 0));
		return 0;
	}
}

/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_snd.c,v 2.1 1995/02/03 15:20:42 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

/*
 * 2DO	add in the T_EXPEDITED
 */


int
t_snd(fd, buf, nbytes, flags)
	int fd;
	char *buf;
	unsigned nbytes;
	int *flags;
{
	int res;
	ts_t *tsp;

	DBG_ENTER(("t_snd",
	    phex, &fd, phex, &buf, phex, &nbytes, phex, &flags, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_snd TBADF", -1, 0));
		return (-1);
	}
	if (tsp->ts_state != T_DATAXFER &&
	    tsp->ts_state != T_INREL) {
		t_errno = TOUTSTATE;
		DBG_EXIT(("t_snd TOUTSTATE", -1, 0));
		return (-1);
	}
		

	if (tsp->ts_protocol != SOCK_STREAM) {
		t_errno = TNOTSUPPORT;
		DBG_EXIT(("t_snd TNOTSUPPORT", -1, 0));
		return (-1);
	}

	/*
	 * NOT YET add in expidited data
	 */

	res = sendto(fd, buf, nbytes, 0, NULL, 0);

	if (res >= 0) {
		DBG_EXIT(("t_snd", res, 0));
		return (res);
	}

	switch (errno) {
	case EWOULDBLOCK:
		t_errno = TNODATA;
		DBG_EXIT(("t_snd TNODATA", -1, 0));
		break;
	case ENOBUFS:
		t_errno = TFLOW;
		DBG_EXIT(("t_snd TFLOW", -1, 0));
		break;
	default:
		t_errno = TSYSERR;
		tli_maperrno();
		DBG_EXIT(("t_snd sendto: TSYSERR", -1, 0));
	}
	return (-1);
}

/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_rcv.c,v 2.1 1995/02/03 15:20:19 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

/*
 * 2DO	add in the T_EXPEDITED
 */


int
t_rcv(fd, buf, nbytes, flags)
	int fd;
	char *buf;
	unsigned nbytes;
	int *flags;
{
	int res;
	ts_t *tsp;

	DBG_ENTER(("t_rcv",
	    phex, &fd, phex, &buf, phex, &nbytes, phex, &flags, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_rcv TBADF", -1, 0));
		return (-1);
	}
	if (tsp->ts_state != T_DATAXFER &&
	    tsp->ts_state != T_OUREL) {
		t_errno = TOUTSTATE;
		DBG_EXIT(("t_rcv TOUTSTATE", -1, 0));
		return (-1);
	}

	if (tsp->ts_protocol != SOCK_STREAM) {
		t_errno = TNOTSUPPORT;
		DBG_EXIT(("t_rcv TNOTSUPPORT", -1, 0));
		return (-1);
	}

	/*
	 * NOT YET add in expidited data
	 */

	res = recvfrom(fd, buf, nbytes, 0, NULL, 0);

	if (res > 0) {
		DBG_EXIT(("t_rcv", res, 0));
		return (res);
	}
	if (res == 0) {
#if 0
		tsp->ts_state = T_INREL;
		t_errno = TLOOK;
#endif
		tsp->ts_eof = 1;
		tsp->ts_event = T_ORDREL; 
		DBG_EXIT(("t_rcv EOF", 0, 0));
		return (0);
	}

	switch (errno) {
	case EWOULDBLOCK:
		t_errno = TNODATA;
		DBG_EXIT(("t_rcv TNODATA", -1, 0));
		return (-1);
	default:
		t_errno = TSYSERR;
		tli_maperrno();
		DBG_EXIT(("t_rcv TSYSERR", -1, 0));
		return (-1);
	}
}

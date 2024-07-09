/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_rcvudata.c,v 2.1 1995/02/03 15:20:34 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

/*
 * 2DO recheck
 */

int
t_rcvudata(fd, up, flags)
	int fd;
	t_unitdata_t *up;
	int *flags;
{
	ts_t *tsp;
	struct sockaddr so;
	int res,len;
	char *buf;
	int size;

	DBG_ENTER(("t_rcvudata", phex, &fd, pt_unitdata, up, phex, &flags, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_rcvudata TBADF", -1, 0));
		return -1;
	}

	/*
	 * NOT NOW	do something about opt
	 */


	if (tsp->ts_protocol != SOCK_DGRAM) {
		t_errno = TNOUDERR;
		DBG_EXIT(("t_rcvudata TNOUDERR", -1, 0));
		return (-1);
	}

	len = up->udata.maxlen;
	buf = up->udata.buf;
	size = sizeof (so);
	sock_init(&so, AF_INET);

	res = recvfrom(fd, buf, len, 0, &so, &size);

	if (res < 0) {
		switch (errno) {
		case EWOULDBLOCK:
			t_errno = TNODATA;
			DBG_EXIT(("t_rcvudata TNODATA:", -1, 0));
			return (-1);
		default:
			t_errno = TSYSERR;
			tli_maperrno();
			DBG_EXIT(("t_rcvudata recfrom: TSYSERR", -1, 0));
			return (-1);
		}
	}
	up->udata.len = res;
	if (up->addr.len != 0)
		if (sock_out(&up->addr, &so)) {
			DBG_EXIT(("t_rcvudata sock_out:", -1, 0));
			return (-1);
		}

	DBG_EXIT(("t_rcvudata", 0, pt_unitdata, up, 0));
	return (0);
}

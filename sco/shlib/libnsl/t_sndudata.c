/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_sndudata.c,v 2.1 1995/02/03 15:20:57 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"


int
t_sndudata(fd, up, flags)
	int fd;
	t_unitdata_t *up;
	int *flags;
{
	ts_t *tsp;
	struct sockaddr ss;
	int res,len;
	char *buf;
	int size;

	DBG_ENTER(("t_sndudata", phex, &fd, pt_unitdata, up, phex, &flags, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_sndudata TBADF", -1, 0));
		return -1;
	}

	/*
	 * NOT NOW	do something about opt
	 */


	if (tsp->ts_protocol != SOCK_DGRAM) {
		t_errno = TNOUDERR;
		DBG_EXIT(("t_sndudata TNOUDERR", -1, 0));
		return (-1);
	}

	if (sock_in(&up->addr, &ss, sizeof (ss), 1)) {
		DBG_EXIT(("t_sndudata sock_in", -1, 0));
		return (-1);
	}
	len = up->udata.len;
	buf = up->udata.buf;
	size = sizeof (ss);

	res = sendto(fd, buf, len, 0, &ss, size);

	if (res < 0) {
		switch (errno) {
		case EWOULDBLOCK:
			t_errno = TNODATA;
			DBG_EXIT(("t_sndudata TNODATA", -1, 0));
			return (-1);

		case ENOBUFS:
			t_errno = TFLOW;
			DBG_EXIT(("t_sndudata TFLOW", -1, 0));
			break;

		default:
			t_errno = TSYSERR;
			tli_maperrno();
			DBG_EXIT(("t_sndudata sendto TSYSERR", -1, 0));
			return (-1);
		}
	}
	up->udata.len = res;
	DBG_EXIT(("t_sndudata", 0, pt_unitdata, up, 0));
	return (0);
}

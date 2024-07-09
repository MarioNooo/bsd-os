/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_listen.c,v 2.1 1995/02/03 15:20:00 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"


int
t_listen(fd, cp)
	int fd;
	call_t *cp;
{
	int res;
	struct sockaddr so;
	ts_t *tsp;
	int i;

	DBG_ENTER(("t_listen", phex, &fd, pt_call, cp, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_listen TBADF", -1, 0));
		return (-1);
	}

	if (cp->addr.maxlen < sizeof (so)) {
		t_errno = TBUFOVFLW;
		DBG_EXIT(("t_listen TBUFOVFLW", -1, 0));
		return (-1);
	}

	if (tsp->ts_protocol != SOCK_STREAM) {
		t_errno = TNOTSUPPORT;
		DBG_EXIT(("t_listen TNOTSUPPORT", -1, 0));
		return (-1);
	}

	cp->opt.len = 0;
	cp->udata.len = 0;

	i = sizeof(so);
	if ((res = accept(fd, &so, &i)) == -1) {
		switch (errno) {
		case EWOULDBLOCK:
			t_errno = TNODATA; /* 2DO return address ?? */
			tsp->ts_state = T_INCON;
			DBG_EXIT(("t_listen T_NODATA", -1, 0));
			break;
		default:
			t_errno = TSYSERR;
			tli_maperrno();
			DBG_EXIT(("t_listen accept: TSYSERR", -1, 0));
			break;
		}
		return (-1);
	}
	tsp->ts_state = T_INCON;
	cp->sequence = res;
	if (cp->addr.maxlen) {
		cp->addr.len = i;
		if (sock_out(&cp->addr, &so)) {
			DBG_EXIT(("t_listen sockout:", -1, 0));
			return (-1);
		}
	}
	DBG_EXIT(("t_listen", res, pt_call, cp, 0));
	return (0);
}

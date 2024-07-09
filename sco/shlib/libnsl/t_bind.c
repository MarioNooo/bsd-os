/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_bind.c,v 2.1 1995/02/03 15:19:28 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

int
t_bind(fd, bip, bop)
	int fd;
	bind_t *bip;
	bind_t *bop;
{
	struct sockaddr si;
	struct sockaddr so;
	int res;
	ts_t *tsp;


	DBG_ENTER(("t_bind", phex, &fd, pt_bind, bip, phex, &bop, 0));

	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_bind TBADF", -1, 0));
		return (-1);
	}

	if (tsp->ts_state != T_UNBND) {
		t_errno = TOUTSTATE;
		DBG_EXIT(("t_bind TOUTSTATE", -1, 0));
		return (-1);
	}

	if (bip != NULL && bip->addr.len != 0) {
		res = sock_in(&bip->addr, &si, sizeof (si), 1);
		if (res < 0) {
			DBG_EXIT(("t_bind sock_in:", -1, 0));
			return (-1);
		}
	} else
		sock_init(&si, AF_INET);

	if (bind(fd, &si, sizeof(si)) == -1) {
		switch (errno) {
		case EADDRNOTAVAIL:
			t_errno = TNOADDR;
			break;
		case EADDRINUSE:
			t_errno = TADDRBUSY;
			break;
		case EACCES:
			t_errno = TACCES;
			break;
		default:
			t_errno = TSYSERR;
			tli_maperrno();
		}
		DBG_EXIT(("t_bind bind: failed", -1, 0));
		return (-1);
	}

	tsp->ts_state = T_IDLE;

	if (bop != NULL) {
		int i = sizeof (so);
		if (getsockname(fd, &so, &i)) {
			t_errno = TSYSERR;
			tli_maperrno();
			DBG_EXIT(("t_bind getsockname: TSYSERR", -1, 0));
			return (-1);
		}
		bop->addr.len = i;
		if (sock_out(&bop->addr, &so)) {
			DBG_EXIT(("t_bind sock_out:", -1, 0));
			return (-1);
		}
	}

	if (bip == NULL || bip->qlen == 0) {
		DBG_EXIT(("t_bind", 0, pt_bind, bop, 0));
		return (0);
	}

	if (res = listen(fd, bip->qlen)) {
		t_errno = TSYSERR;
		tli_maperrno();
		DBG_EXIT(("t_bind listen: TSYSERR", -1, pt_bind, res, 0));
		return (-1);
	}
	if (bop != NULL) {
		if (bip->qlen < 5)
			bop->qlen = bip->qlen;
		else
			bop->qlen = 5;
	}
	DBG_EXIT(("t_bind", 0, pt_bind, bop, 0));
	return (0);
}

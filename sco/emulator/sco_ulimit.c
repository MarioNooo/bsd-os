/*
 * Copyright (c) 1993,1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_ulimit.c,v 2.2 1995/10/06 02:05:21 donn Exp
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include "emulate.h"
#include "sco.h"

/*
 * Support for ulimit emulation.
 */

/* from SCO 3.2v2 man page + SVr4 API ulimit(2) man page */
#define	SCO_UL_GFILLIM	1
#define	SCO_UL_SFILLIM	2
#define	SCO_UL_GMEMLIM	3
#define	SCO_UL_GDESLIM	4

/* 512-byte blocks for UL_[GS]FILLIM */
#define	SCO_BLOCKSHIFT		9
/* XXX maximum non-negative value?  see SVr4 API */
#define	SCO_RLIM_INFINITY	0x7fffffff

int
sco_ulimit(op, arg)
	int op, arg;
{
	struct rlimit rl;
	quad_t flimit;

	switch (op) {

	case SCO_UL_GFILLIM:
		if (getrlimit(RLIMIT_FSIZE, &rl) == -1)
			return (-1);
		flimit = rl.rlim_cur;
		flimit >>= SCO_BLOCKSHIFT;
		if (flimit < 0 || flimit >= SCO_RLIM_INFINITY)
			return (SCO_RLIM_INFINITY);
		return ((int)flimit);

	case SCO_UL_SFILLIM:
		if (arg < 0 || arg >= SCO_RLIM_INFINITY)
			flimit = RLIM_INFINITY;
		else {
			flimit = arg;
			flimit <<= SCO_BLOCKSHIFT;
		}
		/* SVr3.2 recognizes only a hard limit */
		rl.rlim_cur = flimit;
		rl.rlim_max = flimit;
		if (setrlimit(RLIMIT_FSIZE, &rl) == -1)
			return (-1);
		return (0);

	case SCO_UL_GMEMLIM:
		/* XXX we simulate the break, so the rlimit is irrelevant */
		return (SCO_RLIM_INFINITY);

	case SCO_UL_GDESLIM:
		if (getrlimit(RLIMIT_NOFILE, &rl) == -1)
			return (-1);
		if (rl.rlim_cur < 0 || rl.rlim_cur >= SCO_RLIM_INFINITY)
			return (SCO_RLIM_INFINITY);
		return (rl.rlim_cur);
	}

	errno = EINVAL;
	return (-1);
}

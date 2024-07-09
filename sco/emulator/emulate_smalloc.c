/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI emulate_smalloc.c,v 2.1 1995/02/03 15:13:36 polk Exp
 */

#include <sys/param.h>
#include <signal.h>
#include <stdlib.h>

#include "emulate.h"

void *
smalloc(len)
	size_t len;
{
	void *r;
	sigset_t s, os;

	sigfillset(&s);
#ifdef DEBUG
	sigdelset(&s, SIGTRAP);
#endif
	sigprocmask(SIG_SETMASK, &s, &os);
	r = malloc(len);
	sigprocmask(SIG_SETMASK, &os, 0);
	return (r);
}

void
sfree(v)
	void *v;
{
	sigset_t s, os;

	sigfillset(&s);
#ifdef DEBUG
	sigdelset(&s, SIGTRAP);
#endif
	sigprocmask(SIG_SETMASK, &s, &os);
	free(v);
	sigprocmask(SIG_SETMASK, &os, 0);
}

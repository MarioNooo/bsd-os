/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 * 
 * BSDI thread_sigpending.c,v 1.4 2003/07/08 21:52:13 polk Exp
 */

#include <sys/types.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

/*
 * Return a mask containing a list of the pending signals.
 *
 * This call returns an or'd combination of the pending process
 * signals and those queued to the calling pthread.
 */
int
_thread_sys_sigpending(set)
	sigset_t * set;
{
	struct pthread *selfp;

	if (set == NULL)
		return (0);

	selfp = _thread_selfp();

	TR("_thread_sys_sigpending", selfp, 0);

	_thread_kern_enter();
	*set = selfp->sigpend;
	SIGSETOR(*set, _thread_sig_pending);
	_thread_kern_exit();

	return (0);
}

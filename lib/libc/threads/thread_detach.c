/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_detach.c,v 1.7 2002/05/31 14:04:01 ericl Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_detach(pthread)
	pthread_t pthread;
{
	struct pthread *threadp;
	int rc;

	TR("pthread_detach", pthread, 0);

	if (!THREAD_SAFE())
		THREAD_INIT();

	_thread_kern_enter();

	if ((rc = _thread_findp(&threadp, pthread)) != 0)
		goto Return;

	if ((threadp->flags & PTHREAD_DETACHED) != 0)
		goto Return;

	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_DETACH,
	    threadp);
	threadp->flags |= PTHREAD_DETACHED;
	threadp->flags &= ~PTHREAD_ZOMBIE;

	/*
	 * The standard is fuzzy on how to deal with calling detach
	 * when a join had already been done.  I'm going to deal with
	 * it by clearing the JOINED flag and sending a wakeup to 
	 * the thread that had been joined.  The join code notices that
	 * the thread has been detached and returns EINVAL.
	 */
	if ((threadp->flags & PTHREAD_JOINED) != 0) {
		struct pthread * jthread;

		threadp->flags &= ~(PTHREAD_JOINED);
		jthread = threadp->jthread;
		threadp->jthread = 0;
		_thread_kern_setrunnable(jthread);
	}

 Return:
	_thread_kern_exit();

	return (rc);
}

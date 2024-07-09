/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_resume.c,v 1.11 2002/05/31 14:04:01 ericl Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_resume_np(pthread)
	pthread_t pthread;
{
	struct pthread *threadp;
	int rc;

	_thread_kern_enter();

	if ((rc = _thread_findp(&threadp, pthread)) != 0)
		goto Return;

	TR("pthread_resume_np", threadp, threadp->state);

	if (threadp->state == PS_RUNNING)
		goto Done;

	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_RESUME,
	    threadp);
	if (threadp->state == PS_SUSPENDED || threadp->state == PS_RUNNABLE) {
		if (threadp->queue != NULL)
			_thread_queue_remove(threadp->queue, threadp);

		/* might do a task switch so clear the PENDING bit first */
		threadp->flags &= ~(PTHREAD_SUSPEND_PENDING);
		_thread_kern_setrunnable(threadp);
	} else {
		if ((threadp->flags & PTHREAD_SUSPEND_PENDING) == 0) {
			rc = EINVAL;
			goto Return;
		}
	}

 Done:
	threadp->flags &= ~(PTHREAD_SUSPEND_PENDING);

 Return:
	_thread_kern_exit();

	return (rc);
}

/*
 * pthread_resume_all_np() duplicates most all of the code in
 * pthread_resume_np() because it is necessary to change the state in all
 * the threads being resumed BEFORE doing a _thread_kern_exit() as this
 * causes a reschedule.
 */
int
pthread_resume_all_np()
{
	struct pthread *threadp;
	struct pthread *selfp;

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	_thread_kern_enter();
	for (threadp = _thread_allthreads;
	    threadp != NULL;
	    threadp = threadp->nxt) {
	    	if (threadp == selfp)
			continue;
		TR("pthread_resume_all_np", threadp, threadp->state);

		threadp->flags &= ~(PTHREAD_SUSPEND_PENDING);
		if (threadp->state == PS_SUSPENDED ||
		   threadp->state == PS_RUNNABLE) {
			if (threadp->queue != NULL)
				_thread_queue_remove(threadp->queue, threadp);
			_thread_kern_setrunnable(threadp);
		}
	}
	_thread_kern_exit();

	return (0);
}

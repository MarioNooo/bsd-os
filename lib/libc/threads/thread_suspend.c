/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_suspend.c,v 1.9 2002/05/31 14:04:01 ericl Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_suspend_np(pthread)
	pthread_t pthread;
{
	struct pthread *threadp;
	int rc;

	_thread_kern_enter();

	if ((rc = _thread_findp(&threadp, pthread)) != 0)
		goto Return;

	TR("pthread_suspend_np", threadp, threadp->state);
#ifdef	notdef	/* XXX - No such routine as _thread_machdep_getsp() */
	TR("pthread_suspend_np_stack_pc", _thread_machdep_getsp(threadp),
	   _thread_machdep_getpc(threadp));
#endif

	if (threadp == NULL || threadp->state == PS_DEAD) {
		rc = EINVAL;
		goto Return;
	} 

	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_SUSPEND,
	    threadp);
	if (threadp->state == PS_RUNNING) {
		_thread_kern_need_resched = 1;
		_thread_kern_sched_state(threadp, PS_SUSPENDED);
	} else if (threadp->state == PS_RUNNABLE) {
		if (_thread_queue_member(&_thread_runq, threadp))
			_thread_queue_remove(&_thread_runq, threadp);
		_thread_kern_sched_state(threadp, PS_SUSPENDED);
	} else 
		threadp->flags |= PTHREAD_SUSPEND_PENDING;

 Return:
	_thread_kern_exit();

	return (rc);
}

int
pthread_suspend_all_np()
{
	struct pthread *threadp;
	struct pthread *selfp;

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	TR("pthread_suspend_np_all", selfp, 0);

	for (threadp = _thread_allthreads;
	   threadp != NULL;
	   threadp = threadp->nxt) 
		if (threadp != selfp)
			pthread_suspend_np((pthread_t) threadp);
	return (0);
}

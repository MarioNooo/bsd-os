/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_schedparam.c,v 1.7 2000/08/24 15:54:31 jch Exp
 */

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_getschedparam(thread, policy, param)
	pthread_t thread;
	int *policy;
	struct sched_param *param;
{
	struct pthread *threadp;
	int rc;

	TR("pthread_getschedparam", thread, param);

	if (policy == NULL || param == NULL)
		return (EINVAL);

	if (!THREAD_SAFE())
		THREAD_INIT();

	_thread_kern_enter();

	if ((rc = _thread_findp(&threadp, thread)) == 0) {
		(*policy) = threadp->schedpolicy;
		(*param).sched_priority = threadp->priority;
	}

	_thread_kern_exit();

	return (rc);
}

int
pthread_setschedparam(thread, policy, param)
	pthread_t thread;
	int policy;
	const struct sched_param *param;
{
	struct pthread *threadp;
	struct pthread_queue *queue;
	int rc;

	TR("pthread_setschedparam", thread, param);

	if (param->sched_priority < PTHREAD_MIN_PRIORITY ||
	    param->sched_priority > PTHREAD_MAX_PRIORITY)
		return (EINVAL);

	switch (policy) {
	case SCHED_FIFO:
	case SCHED_RR:
	case SCHED_OTHER:
		break;
	default:
		return (EINVAL);
	}

	if (!THREAD_SAFE())
		THREAD_INIT();

	_thread_kern_enter();

	if ((rc = _thread_findp(&threadp, thread)) != 0)
		goto Return;

	threadp->schedpolicy = policy;
	if (threadp->priority != param->sched_priority) {
		/* 
		 * Remove and re-add the thread on whatever queue 
		 * its on so the queue is correctly ordered by priority.
		 */
		threadp->priority = param->sched_priority;
		if ((queue = threadp->queue) != NULL) {
			_thread_queue_remove(queue, threadp);
			_thread_queue_enq(queue, threadp);
		}
		/* 
		 * If we are running, force a trip through the scheduler
		 * to let a higher priority thread run, if any.
		 */
		if (threadp->state == PS_RUNNING)
			_thread_kern_need_resched = 1;
	}

 Return:
	_thread_kern_exit();

	return (rc);
}

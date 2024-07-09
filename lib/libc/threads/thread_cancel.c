/*-
 * Copyright (c) 1999 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_cancel.c,v 1.4 2002/05/31 14:04:00 ericl Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "thread_private.h"
#include "thread_trace.h"

/*
 * Cancel this thread.  Called with the scheduling kernel locked.
 */
void
_thread_cancel(void)
{
	struct pthread *selfp;

	selfp = _thread_selfp();

	TR("_thread_cancel", selfp, selfp->can_state);

	selfp->can_state = PCS_CANCELING;
	_thread_kern_exit();

	pthread_exit(PTHREAD_CANCELED);
}

/*
 * Public entry point to cancel a thread.
 */
int
pthread_cancel(pthread_t pthread)
{
	struct pthread *selfp;
	struct pthread *threadp;
	int rc;

	if (!THREAD_SAFE())
		THREAD_INIT();
	
	TR("pthread_testcancel", threadp, selfp);

	selfp = _thread_selfp();

	_thread_kern_enter();

	if ((rc = _thread_findp(&threadp, pthread)) != 0)
		goto Return;

	if (threadp->state == PS_DEAD) {
		rc = EINVAL;
		goto Return;
	}

	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_CANCEL,
	    threadp);

	switch (pthread->can_flags & PTHREAD_CANCEL_DISABLE) {
	case PTHREAD_CANCEL_DISABLE:
		/* Cancelation is disabled, remember it */
		threadp->can_state = PCS_BLOCKED;
		break;

	case PTHREAD_CANCEL_ENABLE:
		/* Indicate cancelation is pending */
		switch (pthread->can_flags & PTHREAD_CANCEL_ASYNCHRONOUS) {
		case PTHREAD_CANCEL_ASYNCHRONOUS:
			/* Flag it is being canceled */
			threadp->can_state = PCS_ASYNCHRONOUS;
			/* Make sure it is runnable */
			threadp->flags &= ~(PTHREAD_SUSPEND_PENDING);
			_thread_kern_setrunnable(threadp);
			/* Kick us back into the thread monitor */
			_thread_kern_need_resched = 1;
			break;

		case PTHREAD_CANCEL_DEFERRED:
			threadp->can_state = PCS_DEFERRED;
			break;
		}
		break;
	}

 Return:
	_thread_kern_exit();

	return (rc);
}

/*
 * Public entry point to create a cancellation point.
 */
void
pthread_testcancel(void)
{
	struct pthread *selfp;

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	TR("pthread_testcancel", selfp, 0);

	_thread_kern_enter();
	_thread_testcancel();
	_thread_kern_exit();
}

/*
 * Public entry point to set cancellation state.
 */
int
pthread_setcancelstate(int state, int *oldstate)
{
	struct pthread *selfp;

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	TR("pthread_setcancelstate", state, selfp);

	if (state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE)
		return (EINVAL);

	_thread_kern_enter();

	/* Save the old state, if desired */
	if (oldstate != NULL)
		*oldstate = selfp->can_flags & PTHREAD_CANCEL_DISABLE;

	/* Set new cancelation state */
	selfp->can_flags = (selfp->can_flags & ~PTHREAD_CANCEL_DISABLE) | state;

	/* Block a pending cancelation */
	if (state == PTHREAD_CANCEL_DISABLE &&
	    (selfp->can_state == PCS_DEFERRED ||
		selfp->can_state == PCS_ASYNCHRONOUS))
		selfp->can_state = PCS_BLOCKED;

	/* If a pending cancelation was blocked, act on it now */
	if (state == PTHREAD_CANCEL_ENABLE && selfp->can_state == PCS_BLOCKED) {
		/* Pending cancelation, if async do it now, else defer */
		if (selfp->can_flags & PTHREAD_CANCEL_ASYNCHRONOUS)
			_thread_cancel();
		else
			selfp->can_state = PCS_DEFERRED;
	}

	_thread_kern_exit();
	return (0);
}

/*
 * Public entry point to set cancellation type.
 */
int
pthread_setcanceltype(int type, int *oldtype)
{
	struct pthread *selfp;
	
	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	TR("pthread_setcanceltype", type, selfp);

	switch (type) {
	case PTHREAD_CANCEL_DEFERRED:
		break;

	case PTHREAD_CANCEL_ASYNCHRONOUS:
		/* cancel pending, do it now */
		if (selfp->can_state == PCS_DEFERRED ||
		    selfp->can_state == PCS_ASYNCHRONOUS)
			_thread_cancel();
		break;

	default:
		return (EINVAL);
	}

	_thread_kern_enter();

	/* Save the old type, if desired. */
	if (oldtype != NULL)
		*oldtype = selfp->can_flags & PTHREAD_CANCEL_ASYNCHRONOUS;

	/* Update cancelation state */
	selfp->can_flags = (selfp->can_flags & ~PTHREAD_CANCEL_ASYNCHRONOUS) | type;

	/* Switch pending asynchronous cancelation to deferred */
	if (selfp->can_state == PCS_ASYNCHRONOUS && type == PTHREAD_CANCEL_DEFERRED)
		selfp->can_state = PCS_DEFERRED;

	_thread_kern_exit();
	return (0);
}


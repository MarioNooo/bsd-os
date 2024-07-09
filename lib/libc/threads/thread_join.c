/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_join.c,v 1.7 2003/06/17 18:06:07 giff Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_join(pthread, result)
	pthread_t pthread;
	void ** result;
{
	struct pthread *selfp;
	struct pthread *threadp;
	int rc;

	if (!THREAD_SAFE())
		THREAD_INIT();

	TR("pthread_join", pthread, result);

	_thread_kern_enter();

	if ((rc = _thread_findp(&threadp, pthread)) != 0)
		goto Return;

	/* Can't call join on ourselves */
	if (threadp == (selfp = _thread_selfp())) {
		rc = EDEADLK;
		goto Return;
	}

	/* Check if this thread has been detached */
	if (((threadp->flags & PTHREAD_DETACHED) != 0) ||
	    ((threadp->flags & PTHREAD_JOINED) != 0)) {
		rc = EINVAL;
		goto Return;
	}

	threadp->flags |= PTHREAD_JOINED;
	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_JOIN,
	    threadp);

	/* 
	 * If the thread isn't dead yet we'll block and wait for 
	 * them to make us runnable from pthread_exit(). 
	 */
	if (threadp->state != PS_DEAD) {
		threadp->jthread = selfp;	/* So they can signal us */
		_thread_kern_block(NULL, PS_JOIN_WAIT, NULL);

		TR("pthread_join_blkret1", pthread, threadp);

		/* Check if the thread is detached */
		if ((threadp->flags & PTHREAD_DETACHED) != 0) {
			rc = EINVAL;
			goto Return;
		}

	} 

	if (result != NULL)
		*result = threadp->status;

	/* We've retrieved the status so clear the zombie flag */
	threadp->flags &= ~(PTHREAD_ZOMBIE);
	_thread_kern_cleanup_dead();

 Return:
	_thread_kern_exit();

	return (rc);
}

/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_exit.c,v 1.10 2002/05/31 14:04:01 ericl Exp
 */

#include <sys/types.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "thread_private.h"
#include "thread_trace.h"

extern int _thread_count;	/* count of all threads for signals */

void           
_thread_exit_wdb_hook(thread)
	struct pthread * thread;
{
        ptrace(PT_EVENT, 0, (caddr_t) thread, PT_EVT_WDB_EXIT);
}


void
pthread_exit(status)
	void * status;
{
	struct pthread * selfp;

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	TR("pthread_exit", status, selfp);
	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_EXIT, status);

	_thread_kern_enter();
	selfp->status = status;

	while (selfp->cleanup != NULL) {
		_thread_kern_exit();
		pthread_cleanup_pop(1);
		_thread_kern_enter();
	}

	/* Clean up any thread specific data */
	_thread_key_cleanupspecific();

	/* 
	 * Remove ourselves from the list of all active threads 
	 * add to the dead thread list. We'll live there until 
	 * finally cleaned up by the pthread kernel.
	 */

	_thread_list_delete(&_thread_allthreads, selfp);
	_thread_list_add(&_thread_deadthreads, selfp);

	/* Handle detach/join cleanup issues */
	if ((selfp->flags & PTHREAD_DETACHED) == 0) {
		/* Say we are waiting to have our status retrieved */
		selfp->flags |= PTHREAD_ZOMBIE;	

		/* If a thread is blocked on a join make them runnable */
		if ((selfp->flags & PTHREAD_JOINED) != 0)
			_thread_kern_setrunnable(selfp->jthread);
	}

	/* indicate we are history */
	_thread_kern_sched_state(selfp, PS_DEAD);
	--_thread_count;

	/* update the signal counts */
	_thread_sigmask(SIG_UNBLOCK, &selfp->sigmask, 0);

	_thread_queue_remove(&_thread_runq, selfp);

	_thread_exit_wdb_hook (selfp);

	_thread_kern_need_resched = 1;
	_thread_kern_exit();

	/* This point should not be reached. */
	kill(getpid(), SIGABRT);

	/* NOTREACHED */
}

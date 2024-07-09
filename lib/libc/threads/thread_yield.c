/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_yield.c,v 1.7 2001/10/03 17:29:55 polk Exp
 */

#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

/*
 * Externally visible function to force a reschedule of the
 * calling thread.  
 */
int
_thread_sys_sched_yield()
{

	TR("sched_yield", 0, 0);

	if (_thread_selfp() == NULL)
		return (_syscall_sys_sched_yield());

	_thread_kern_enter();
	_thread_kern_need_resched = 1;
	_thread_kern_exit();
	return (0);
}

/*
 * pthread_yield() was defined in older pthreads drafts.  I include
 * it here as an aid to compiling older programs which still use it
 * instead of the POSIX.4 sched_yield() function.
 */
int
pthread_yield()
{

	return (sched_yield());
}

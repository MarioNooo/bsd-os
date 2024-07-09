/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_getstack.c,v 1.6 2000/08/24 15:54:29 jch Exp
 */

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_getstacksize_np(thread, stacksize)
	pthread_t thread;
	int *stacksize;
{
	struct pthread *threadp;
	int rc;

	TR("pthread_getstacksize_np", thread, stacksize);

	if (stacksize == NULL)
		return (EINVAL);

	_thread_kern_enter();

	if ((rc = _thread_findp(&threadp, thread)) == 0)
		*stacksize = threadp->stacksize;

	_thread_kern_exit();

	return (rc);
}

int
pthread_getstackbase_np(thread, stackbase)
	pthread_t thread;
	int *stackbase;
{
	struct pthread *threadp;
	int rc;

	TR("pthread_getstackbase_np", thread, stackbase);

	if (stackbase == NULL)
		return (EINVAL);

	_thread_kern_enter();

	if ((rc = _thread_findp(&threadp, thread)) == 0)
		*stackbase = (int)threadp->stackaddr;

	_thread_kern_exit();

	return (rc);
}

int
pthread_getstackpointer_np(thread, stackpointer)
	pthread_t thread;
	int *stackpointer;
{
	struct pthread *threadp;
	int rc;

	TR("pthread_getstackpointer_np", thread, stackpointer);

	if (stackpointer == NULL)
		return (EINVAL);

	_thread_kern_enter();

	if ((rc = _thread_findp(&threadp, thread)) == 0)
		*stackpointer = (int)_thread_machdep_getsp(threadp);

	_thread_kern_exit();

	return (rc);
}

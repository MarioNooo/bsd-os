/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 * 
 * BSDI thread_self.c,v 1.3 2000/08/24 15:54:31 jch Exp
 */

/*
 * Return versions of "the current thread" to the caller.
 * 
 * The current implementation implements a pthread_t indentifier
 * simple as a pointer to the thread data structure. This file
 * implements both an external callable interface and an 
 * internal one.  
 */

#include <errno.h>
#include <pthread.h>

#include "thread_private.h"

/*
 * Public routine to return the thread identifier of the
 * calling thread.  In this implementation a pthread_t
 * is currently a pointer so its the same as _thread_selfp();
 */
pthread_t
pthread_self(void)
{
      if (!THREAD_SAFE())
              THREAD_INIT();
	return (_thread_selfp());
}


/*
 * Internal routine to translate from a thread id to a thread pointer.
 * Its currently easy but will change when pthread_t becomes an integer id.
 */

int
_thread_findp(threadp, pthread)
	struct pthread **threadp;
	pthread_t pthread;
{
	struct pthread *tp;

	if (pthread == NULL)
		return (EINVAL);

	for (tp = _thread_allthreads; tp != NULL; tp = tp->nxt) {
		if (tp != pthread)
			continue;

		*threadp = tp;
		return (0);
	}

	for (tp = _thread_deadthreads; tp != NULL; tp = tp->nxt) {
		if (tp != pthread)
			continue;

		*threadp = tp;
		return (0);
	}

	return (ESRCH);
}

/*
 * Internal routine to return a pointer to the current thread data
 * structure.
 */

#undef _thread_selfp
struct pthread *
_thread_selfp(void)
{
	return (_thread_run);	/* XXX single CPU only */
}

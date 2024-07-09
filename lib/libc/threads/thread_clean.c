/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_clean.c,v 1.3 1999/10/04 16:20:39 jch Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_cleanup_push(routine, arg)
	void 	(*routine) (void *);
	void *	arg;
{
	struct pthread * selfp;
	struct pthread_cleanup *new;

	TR("pthread_cleanup_push", routine, arg);

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	if ((new = malloc(sizeof(struct pthread_cleanup))) == NULL)
		return (ENOMEM);

	new->routine = routine;
	new->arg = arg;

	_thread_kern_enter();

	new->next = selfp->cleanup;
	selfp->cleanup = new;

	_thread_kern_exit();

	return (0);
}

void
pthread_cleanup_pop(execute)
	int 	execute;
{
	struct pthread * selfp;
	struct pthread_cleanup * top;

	TR("pthread_cleanup_pop", execute, 0);

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	_thread_kern_enter();
	if ((top = selfp->cleanup) != NULL) {
		selfp->cleanup = top->next;
		_thread_kern_exit();
		if (execute) 
			top->routine(top->arg);
		free(top);
		return;
	}
	_thread_kern_exit();
	return;
}

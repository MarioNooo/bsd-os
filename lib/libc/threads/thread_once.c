/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_once.c,v 1.5 1999/10/07 18:24:15 jch Exp
 */

#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"

/*
 * Handle one time initialization for the pthreads library.
 * Calls the user provided initialization routine at most
 * once, insuring that library routines have been correctly 
 * initialized.
 *
 * Assumes that the thread kernel is UNLOCKED upon entry
 */
int
pthread_once(once_control, init_routine)
	pthread_once_t * once_control;
	void (*init_routine) __P((void));
{

ploop:
	_thread_kern_enter();
	if (once_control->state == PTHREAD_NEEDS_INIT) {
		once_control->state = PTHREAD_INIT_INIT;
		_thread_kern_exit();
		init_routine();
		_thread_kern_enter();
		once_control->state = PTHREAD_DONE_INIT;
	}
	else if (once_control->state == PTHREAD_INIT_INIT) {
		_thread_kern_need_resched++;
		_thread_kern_exit();
		goto ploop;
	}
	_thread_kern_exit();
	return (0);
}

/*
 * Assumes that the thread kernel is LOCKED upon entry
 */
int
_thread_once(once_control, init_routine)
	pthread_once_t * once_control;
	void (*init_routine) __P((void));
{
_loop:
	if (once_control->state == PTHREAD_NEEDS_INIT) {
		once_control->state = PTHREAD_INIT_INIT;
		init_routine();
		once_control->state = PTHREAD_DONE_INIT;
	}
	else if (once_control->state == PTHREAD_INIT_INIT) {
		_thread_kern_need_resched++;
		_thread_kern_exit();
		goto _loop;
	}
	return (0);
}

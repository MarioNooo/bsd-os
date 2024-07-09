/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_atfork.c,v 1.4 2000/07/25 01:24:05 jch Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

struct pthread_atfork *_thread_fork_prepare 	= NULL;
struct pthread_atfork *_thread_fork_child 	= NULL;
struct pthread_atfork *_thread_fork_parent	= NULL;


int
_thread_atfork(prepare, parent, child)
	void (*prepare) __P((void));
	void (*parent) __P((void));
	void (*child) __P((void));
{
	struct pthread_atfork *new, *next;

	if (prepare != NULL) {
		if ((new = malloc(sizeof(struct pthread_atfork))) == NULL) {
			_thread_kern_exit();
			return (ENOMEM);
		}
		new->routine = prepare;
		new->next = _thread_fork_prepare;
		_thread_fork_prepare = new;
	}

	if (parent != NULL) {
		if ((new = malloc(sizeof(struct pthread_atfork))) == NULL) {
			_thread_kern_exit();
			return (ENOMEM);
		}
		new->routine = parent;
		new->next = NULL;
		if (_thread_fork_parent == NULL)
			_thread_fork_parent = new;
		else {
			next = _thread_fork_parent;
			while (next->next != NULL)
				next = next->next;
			next->next = new;
		}
	}

	if (child != NULL) {
		if ((new = malloc(sizeof(struct pthread_atfork))) == NULL) {
			_thread_kern_exit();
			return (ENOMEM);
		}
		new->routine = child;
		new->next = NULL;
		if (_thread_fork_child == NULL)
			_thread_fork_child = new;
		else {
			next = _thread_fork_child;
			while (next->next != NULL)
				next = next->next;
			next->next = new;
		}
	}

	return (0);
}

/*
 * The prepare list is maintained in LIFO order while the
 * child and parent lists are in FIFO order.  The standard
 * wants them called this way and it simpleifies the code
 * in fork which just traverses the lists calling functions.
 */
int
pthread_atfork(prepare, parent, child)
	void (*prepare) __P((void));
	void (*parent) __P((void));
	void (*child) __P((void));
{
	int rc;

	TR("pthread_atfork", parent, child);

	if (!THREAD_SAFE())
		THREAD_INIT();

	_thread_kern_enter();

	rc = _thread_atfork(prepare, parent, child);

	_thread_kern_exit();

	return (rc);
}

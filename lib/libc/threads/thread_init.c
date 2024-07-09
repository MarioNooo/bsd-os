/*
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_init.c,v 1.8 2002/06/20 13:59:24 ericl Exp 
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

pthread_once_t _thread_init_once = PTHREAD_ONCE_INIT;
int _threads_initialized = 0;
struct pthread *_thread_initial = NULL;

void
_thread_init(void)
{

	/* 
	 * Check if this function has already been called:
	 * Only initialise the threaded application once. 
	 */
	if (THREAD_SAFE())
		return;

	TR("_thread_init", 0, 0);

	if (_thread_kern_init() != 0)
		kill(getpid(), SIGABRT);
	if (_thread_sig_init() != 0)
		kill(getpid(), SIGABRT);
	if (_thread_aio_table_init() != 0)
		kill(getpid(), SIGABRT);

	/*
	 * Call low level thread creation routine with a selfp of NULL
	 * to indicate we are creating a thread structure for the 
	 * initial thread.  The create code handles this specially since
	 * we are already running, don't require a stack, etc.
	 */

	if (_thread_create(NULL, &_thread_initial, NULL, NULL, NULL) != 0)
		kill(getpid(), SIGABRT);

	_threads_initialized = 1;

	return;
}

/*
 * This is provided for debugging and for older applications that call it.
 */

int pthread_init()
{

	if (!THREAD_SAFE())
		THREAD_INIT();

	TR("pthread_init", 0, 0);

	return (0);
}

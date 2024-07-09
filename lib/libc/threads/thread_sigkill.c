/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_sigkill.c,v 1.4 2000/08/24 15:54:31 jch Exp
 */

#include <sys/types.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_kill(tid, signo)
	pthread_t tid;
	int signo;
{
	struct pthread * threadp;
	struct pthread * current;
	int ret;

	TR("pthread_kill", tid, signo);

	_thread_kern_enter();

	if ((ret = _thread_findp(&threadp, tid)) == 0)
		ret = _thread_sig_post(threadp, signo);

	_thread_kern_exit();

 	return (ret);
}

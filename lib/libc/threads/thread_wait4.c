/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 * 
 * BSDI thread_wait4.c,v 1.4 2003/06/17 18:06:07 giff Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

/* 
 * Perform a non-blocking wait system call.  We'll poll for an exited
 * child initially.  If none is found we block until we get a SIGCHLD
 * signal.  
 */
pid_t
_thread_sys_wait4(pid, istat, options, rusage)
	pid_t pid;
	int *istat;
	int options;
	struct rusage *rusage;
{
	pid_t erc;

	TR("_thread_sys_wait4", pid, istat);

	_thread_kern_enter();

	while (1) {
		errno = 0;
		erc = _syscall_sys_wait4(pid, istat, options | WNOHANG, rusage);
		if ((erc != 0) || ((options & WNOHANG) != 0)) 
			break;

		if (_thread_kern_block(&_thread_waitq, PS_WAIT_WAIT, NULL) == 0)
			continue;

		erc = -1;
		break;
	}

	_thread_kern_exit();

	return (erc);
}

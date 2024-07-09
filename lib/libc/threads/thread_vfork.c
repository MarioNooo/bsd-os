/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_vfork.c,v 1.3 2000/09/19 18:31:40 jch Exp
 */

#include <sys/types.h>
#include <sys/sfork.h>
#include <sys/syscall.h>

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"

extern sigset_t _thread_proc_sig_blocked;

pid_t
_thread_sys_vfork(void)
{
	pid_t pid;

	_thread_kern_enter();

	/*
	 * Threads are not vfork() safe so we'll need to skip the
	 * shared memory.
	 */
	pid = sfork(SF_WAITCHILD, NULL, SIGCHLD);

	_thread_kern_exit();

	return (pid);
}

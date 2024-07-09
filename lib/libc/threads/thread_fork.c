/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_fork.c,v 1.5 2003/07/08 21:52:13 polk Exp
 */

#include <sys/types.h>

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"

pid_t
_thread_sys_fork(void)
{
	struct pthread * selfp;
	struct pthread_atfork *atfork;
	pid_t pid;

	selfp = _thread_selfp();

	TR("_thread_sys_fork", selfp, 0);

	_thread_kern_enter();
	atfork = _thread_fork_prepare;
	while (atfork != NULL) {
		TR("_thread_sys_fork atfork", selfp, atfork);
		_thread_kern_exit();
		atfork->routine();
		_thread_kern_enter();
		atfork = atfork->next;
	}

	pid = _syscall_sys_fork();

	if (pid == -1) {
		TR("_thread_sys_fork failure", selfp, pid);
		_thread_kern_exit();
		return (pid);
	}

	if (pid != 0) {
		/* Parent */
		TR("_thread_sys_fork parent", selfp, pid);
		atfork = _thread_fork_parent;
		while (atfork != NULL) {
			TR("_thread_sys_fork atfork", selfp, atfork);
			_thread_kern_exit();
			atfork->routine();
			_thread_kern_enter();
			atfork = atfork->next;
		}
		TR("_thread_sys_fork parent done", selfp, pid);
		_thread_kern_exit();
		return (pid);
	}

	TR("_thread_sys_fork child", selfp, pid);

	/* Child */
	SIGEMPTYSET(selfp->sigpend);	/* No pending signals */

	/* Unceremoniously kill any other threads */
	_thread_kern_cleanup_child();

	/* Tell ptrace about our threads */
	_thread_create_debugger_hook();

	atfork = _thread_fork_child;
	while (atfork != NULL) {
		TR("_thread_sys_fork atfork", selfp, atfork);
		_thread_kern_exit();
		atfork->routine();
		_thread_kern_enter();
		atfork = atfork->next;
	}

	TR("_thread_sys_fork child done", selfp, pid);

	_thread_kern_exit();
	return (pid);
}

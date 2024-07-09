/*-
 * Copyright (c) 1999 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI thread_execve.c,v 1.5 2003/07/08 21:52:13 polk Exp
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "thread_private.h"
#include "thread_signal.h"
#include "thread_trace.h"

extern sigset_t _thread_proc_sig_blocked;

/*
 * Fix up signals and timers before performing the real execve() call.
 */
int
_thread_sys_execve(path, argv, envp)
	const char *path;
	char *const *argv;
	char *const *envp;
{
	struct itimerval itv;
	struct itimerval oitv;
	struct sigaction sa;
	struct sigaction osa;
	struct pthread *selfp;
	int i;
	int error;

	_thread_kern_enter();

	selfp = _thread_selfp();

	TR("_thread_sys_execve", selfp, path);

	if (sigvector[IDX(SIGALRM)].sa_handler == SIG_IGN) {
		/* We're supposed to toss alarms; do it for real.  */
		sa.sa_handler = SIG_IGN;
		SIGEMPTYSET(sa.sa_mask);
		sa.sa_flags = 0;
		if (_syscall_sys_sigaction(SIGALRM, &sa, &osa) == -1) {
			_thread_kern_exit();
			return (-1);
		}
	} else {
		/* Read the current alarm value and install it officially.  */
		_thread_getitimer(&itv);
		if (_syscall_sys_setitimer(ITIMER_REAL, &itv, &oitv) == -1) {
			_thread_kern_exit();
			return (-1);
		}
	}

	/* Block signals for real.  */
	if (SIGSETNEQ(selfp->sigmask,_thread_proc_sig_blocked))
		_syscall_sys_sigprocmask(SIG_SETMASK, &selfp->sigmask, NULL);

	/* Restore file descriptor flags.  */
	for (i = 0; i < _thread_aio_table_size; ++i) {
		thread_aio_desc_t *dp;

		if ((dp = _thread_aio_table[i]) != NULL &&
		    dp->aiofd_object != NULL &&
		    (dp->aiofd_fdflags & FD_NONBLOCK) == 0)
			_syscall_sys_fcntl(i, F_SETFD, dp->aiofd_fdflags);
	}

	_syscall_sys_execve(path, argv, envp);

	/*
	 * Failure.  Do our best to recover.
	 */
	error = errno;

	for (i = 0; i < _thread_aio_table_size; ++i) {
		thread_aio_desc_t *dp;

		if ((dp = _thread_aio_table[i]) != NULL &&
		    dp->aiofd_object != NULL &&
		    (dp->aiofd_fdflags & FD_NONBLOCK) == 0)
			_syscall_sys_fcntl(i, F_SETFD,
			    dp->aiofd_fdflags | FD_NONBLOCK);
	}

	if (SIGSETNEQ(selfp->sigmask,_thread_proc_sig_blocked))
		_syscall_sys_sigprocmask(SIG_SETMASK, &_thread_proc_sig_blocked,
		    NULL);

	if (sigvector[IDX(SIGALRM)].sa_handler == SIG_IGN)
		_syscall_sys_sigaction(SIGALRM, &osa, NULL);
	else
		_syscall_sys_setitimer(ITIMER_REAL, &oitv, NULL);

	errno = error;

	_thread_kern_exit();

	return (-1);
}

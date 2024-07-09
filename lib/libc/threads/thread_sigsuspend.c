/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_sigsuspend.c,v 1.17 2003/07/08 21:52:13 polk Exp
 */

#include <sys/types.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"
#include "thread_signal.h"

extern sigset_t _thread_cantmask;

/*
 * Install a new signal mask and "wait" for a signal to arrive.
 * If the new mask allows a pending signal to be delivered we
 * won't bother to block at all we'll just deliver the signal
 * and return.
 */
int
_thread_sys_sigsuspend(set)
	const sigset_t * set;
{
	struct pthread *selfp;
	sigset_t oset, stmp;

	_thread_kern_enter();

	selfp = _thread_selfp();

	TR("_thread_sys_sigsuspend", *set, selfp);

	selfp->flags &= ~PTHREAD_INTERRUPTED;

	/* Save the current mask and install the temporary one */
	_thread_sigmask(SIG_BLOCK, set, &oset);

	/* 
	 * If we have a pending signal that is deliverable with the
	 * new mask do it now and don't bother to block.  Otherwise we'll
	 * block and wait for one to arrive. Don't deliver signals pending
	 * on the process, just this thread.
	 *
	 * We may unblock for a signal that is masked. In that case we
	 * check the mask and try again.
	 */
	stmp = selfp->sigpend;
	SIGSETANDC(stmp, selfp->sigmask);
	while (SIGISEMPTY(stmp)) {
		_thread_kern_block(NULL, PS_SIG_WAIT, NULL);
		TR("_thread_sys_sigsuspend_blkret1", selfp, 0);
		if (selfp->flags & PTHREAD_INTERRUPTED)
			break;
		stmp = selfp->sigpend;
		SIGSETANDC(stmp, selfp->sigmask);
	}

	SIGEMPTYSET(stmp);
	_thread_sig_deliver_pending(stmp, 0, 0, 1);

	/* Restore the signal mask: */
	_thread_sigmask(SIG_SETMASK, &oset, 0);
	errno = EINTR;
	_thread_kern_exit();

	return (-1);
}

/*
 * Wait for a signal to arrive.
 */
int
_thread_sys_sigwait(set, sig)
	const sigset_t *set;
	int	*sig;
{
	struct pthread *selfp;
	struct 	sigaction ours;
	struct sigaction sigsave[NSIG];
	sigset_t found, waitfor, myset, stmp;
	int signo, i;

	selfp = _thread_selfp();
	myset = *set;

	TR("_thread_sys_sigwait", selfp, 0);

	if (set == 0 || SIGISEMPTY(myset))
		return (EINVAL);

	_thread_kern_enter();

	found = _thread_sig_pending;
	SIGSETOR(found, selfp->sigpend);
	SIGSETAND(found, myset);
	if (SIGNOTEMPTY(found))
		goto gotone;

	/*
	 * We will have to wait - insure that all signals waited for have
	 * the pthreads signal handler installed.
	 */

	for (signo = 1; signo < NSIG; signo++) {
		if (!SIGISMEMBER(myset, signo))
		    continue;

		if (SIGISMEMBER(_thread_cantmask, signo))
			continue;

		sigsave[IDX(signo)] = sigvector[IDX(signo)];
		sigvector[IDX(signo)].sa_handler = SIG_ERR;

		if (signo == SIGALRM || signo == SIGTRAP)
			continue;

		if (sigsave[IDX(signo)].sa_handler == SIG_IGN ||
		    sigsave[IDX(signo)].sa_handler == SIG_DFL) {
			SIGEMPTYSET(ours.sa_mask);
			ours.sa_flags = SA_RESTART;
			ours.sa_handler = (void (*)()) _thread_sig_handler;
			TR("_thread_sys_sigwait_sigaction1", signo, selfp);
			(void) _syscall_sys_sigaction(signo, &ours, NULL);
		}
	}

	waitfor = myset;
	SIGSETOR(waitfor, _thread_cantmask);

	/* 
	 * While one of the signals we are waiting for is not pending, block.
	 */
	found = _thread_sig_pending;
	SIGSETOR(found, selfp->sigpend);
	SIGSETAND(found, waitfor);
	while (SIGISEMPTY(found)) {
		_thread_kern_block(NULL, PS_SIG_WAIT, NULL);
		TR("_thread_sys_sigwait_blkret1", selfp, 0);
		if (selfp->flags & PTHREAD_INTERRUPTED)
			break;
		found = _thread_sig_pending;
		SIGSETOR(found, selfp->sigpend);
		SIGSETAND(found, waitfor);
	}

	/* Restore prior signal handlers */
	for (signo = 1; signo < NSIG; signo++) {
		if (!SIGISMEMBER(myset, signo))
			continue;

		if (SIGISMEMBER(_thread_cantmask, signo))
			continue;

		sigvector[IDX(signo)] = sigsave[IDX(signo)];

		if (signo == SIGALRM || signo == SIGTRAP)
			continue;

		if (sigvector[IDX(signo)].sa_handler == SIG_IGN ||
		    sigvector[IDX(signo)].sa_handler == SIG_DFL) {
			TR("_thread_sys_sigwait_sigaction2", signo, selfp);
			(void) _syscall_sys_sigaction(signo,
			    &sigvector[IDX(signo)], NULL);
		}
	}

gotone:
	TR("_thread_sys_sigwait gotone", selfp->flags & PTHREAD_INTERRUPTED, 0);
	stmp = found;
	SIGSETANDC(stmp, _thread_cantmask);
	if (SIGISEMPTY(stmp)) {
		TR("_thread_sys_sigwait EINTR", 0, 0);
		_thread_kern_exit();
		return (EINTR);
	}

	for (i=1; i<NSIG; i++)
		if (SIGISMEMBER(found, i))
			break;

	SIGDELSET(_thread_sig_pending, i);
	SIGDELSET(selfp->sigpend, i);

	if (sig)
		*sig = i;

	_thread_seterrno(selfp, 0);
	TR("_thread_sys_sigwait returns", i, 0);
	_thread_kern_exit();

	return (0);
}

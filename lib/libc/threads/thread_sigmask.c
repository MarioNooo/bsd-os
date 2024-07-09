/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_sigmask.c,v 1.14 2003/07/08 21:52:13 polk Exp
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

extern sigset_t _thread_proc_sig_blocked;	/* Signals blocked at process */
extern sigset_t _thread_proc_sig_blocked_orig;
extern sigset_t _thread_cantmask;
extern int *_thread_sigcount;	/* per signal count of threads masks */
extern int _thread_count;	/* count of active threads */

int
pthread_sigmask(how, set, oset)
	int 	how;
	const 	sigset_t * set;
	sigset_t * oset;
{
	int erc;

	if (!THREAD_SAFE())
		THREAD_INIT();

	_thread_kern_enter();
 	TR("pthread_sigmask", how, 0);
 	erc = _thread_sigmask(how, set, oset);
	TR("pthread_sigmask_end", 0, 0);
	_thread_kern_exit();
	return (erc);
}

/*
 * _thread_sigmask() - callable ONLY from within the thread library
 * Assumes that the thread kenel is ALREADY LOCKED!
 */
int
_thread_sigmask(how, set, oset)
	int 	how;
	const 	sigset_t *set;
	sigset_t * oset;
{
	struct pthread *selfp;
	int erc;
	int i;
	sigset_t newset, diff, oldset, stmp, emptyset;

	selfp = _thread_selfp();

	erc = 0;

 	TR("_thread_sigmask", how, 0);

 	/* Check if the existing signal process mask is to be returned: */
 	if (oset != NULL) {
 		*oset = selfp->sigmask;
 		TR("_thread_sigmask_old", 0, 0);
 	}
 	oldset = selfp->sigmask;
 
	if (set != NULL)  {
 		TR("_thread_sigmask_new", how, 0);
		if (how == SIG_SETMASK) {
			selfp->sigmask = *set;
			SIGSETANDC(selfp->sigmask, _thread_cantmask);
		}
		else if (how == SIG_BLOCK) {
			stmp = *set;
			SIGSETANDC(stmp, _thread_cantmask);
			SIGSETOR(selfp->sigmask, stmp);
		}
		else if (how == SIG_UNBLOCK)
			SIGSETANDC(selfp->sigmask, *set);
		else
			erc = EINVAL;
	}

 	if (erc)
		SIGEMPTYSET(*oset);
 
	/*
	 * Start with newset equal to the existing process sigmask.
	 * Check if the sigmask changed. If not, then do nothing.
	 * For each signal; If it hasn't changed, goto next signal
	 *  if the signal is set on the old mask (then it is NOT set in the
	 *    new) then delete it from newset. Decrement the count for this
	 *    signal in _thread_sigcount, checking for < 0 situations.
	 *  if the signal is NOT set in the old mask (then it IS set in the
	 *    new) then increment the count for this signal in
	 *    _thread_sigcount. If
	 *    count for this signal is greater or equal to the number of
	 *    threads then all threads must have this signal masked and it is
	 *    therefore safe to mask it at the process level, set the signal in
	 *    newset. Check for the signal count greater than thread count.
	 */
	newset = _thread_proc_sig_blocked;
	SIGEMPTYSET(diff);
	for (i = 1; i < NSIG; i++) {
		if (SIGISMEMBER(oldset, i) && !SIGISMEMBER(selfp->sigmask, i))
			SIGADDSET(diff, i);
		if (!SIGISMEMBER(oldset, i) && SIGISMEMBER(selfp->sigmask, i))
			SIGADDSET(diff, i);
	}
	if (SIGNOTEMPTY(diff)) {
 		TR("_thread_sigmask_newset", 0, 0);
		for (i = 1; i < NSIG; i++) {
			if (!SIGISMEMBER(diff, i))
				continue;
			if (SIGISMEMBER(_thread_cantmask, i))
				continue;
			if (SIGISMEMBER(oldset, i)) {
				SIGDELSET(newset, i);
				if (--_thread_sigcount[i] < 0) {
#ifdef DEBUG
					printf("_thread_sigcount below zero for thread 0x%x and signal %d\n", selfp, i);
#endif
					_thread_sigcount[i] = 0;
				}
			}
			else {
				if (++_thread_sigcount[i] >= _thread_count) {
					SIGADDSET(newset, i);
					if (_thread_sigcount[i] > _thread_count) {
#ifdef DEBUG
						printf("_thread_sigcount above thread count for thread 0x%x and signal %d\n", selfp, i);
#endif
						_thread_sigcount[i] = _thread_count;
					}
				}
			}
		}
	}

	/*
	 * We have a new mask now for the process. We can never mask signals
	 * at the process which were never masked in the first place so strip
	 * out the unwanted ones now.  Check to see if the new mask is
	 * different than the old one and must be updated.
	 */
	SIGSETAND(newset, _thread_proc_sig_blocked_orig);
	if (SIGSETNEQ(newset, _thread_proc_sig_blocked)) {
 		TR("_thread_sigmask_set proc", 0, 0);
		_syscall_sys_sigprocmask(SIG_SETMASK, &newset, NULL);
		_thread_proc_sig_blocked = newset;
	}

 	/*
 	 * Deliver any pending signals. It doesn't matter that they may 
 	 * be masked by this thread. If any are able to be delivered to
  	 * another thread then that is OK too.
 	 */
	SIGEMPTYSET(emptyset);
 	_thread_sig_deliver_pending(emptyset, 0, 0, 1);
  
	TR("_thread_sigmask_end", selfp, 0);
	return (erc);
}

/*
 * Threaded version of sigprocmask.
 *
 * According to the standard, calling sigprocmask in a threaded
 * program is undefined.  In order to increase compatability
 * I define sigprocmask in terms of pthread_sigmask().
 */

int
_thread_sys_sigprocmask(how, set, oset)
	int how;
	const sigset_t * set;
	sigset_t * oset;
{
	int     ret;

	TR("_thread_sys_sigprocmask", how, 0);

	if ((ret = pthread_sigmask(how, set, oset)) != 0) {
		errno = ret;
		return (-1);
	}
	return (0);
}

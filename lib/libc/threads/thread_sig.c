/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_sig.c,v 1.26 2003/07/08 21:52:13 polk Exp
 */

#include <sys/types.h>

#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"
#include "thread_signal.h"

/* Forward declarations */
static int _thread_sig_deliver_one __P((int, int, struct sigcontext *, int));

sigset_t _thread_sig_blocked;		/* Signals arrived while kernel lock */
sigset_t _thread_sig_pending;		/* Signals pending delivery at proc */
struct sigaction * sigvector;		/* The signal vector */
sigset_t _thread_proc_sig_blocked;	/* Signals blocked at the process */
sigset_t _thread_proc_sig_blocked_orig;
int *_thread_sigcount;			/* per signal count of threads masks */
sigset_t _threadspec_sigmask;
sigset_t _thread_cantmask;

#define	IDX(x)			((x) - 1)

/*
 * Initialize the signal handling system for threads.  The
 * signal vector is per process so we manage a global one here.
 * Signal masks are maintained per thread and we store a copy
 * in the pthread structure.  The sa_mask field is or'd with the
 * per thread signal mask when the signal is delivered.
 */

int
_thread_sig_init()
{
	struct sigaction act;
	int sig;
	sigset_t _proc_sig_blocked;

	TR("_thread_sig_init", 0, 0);

	/* Allocate signal vector for user signals */
	if ((sigvector = calloc(NSIG, sizeof(struct sigaction)))
	   == NULL)
		return (ENOMEM);

	/* Allocate signal count array */
	if ((_thread_sigcount = calloc(NSIG + 1, sizeof(int))) == NULL)
		return (ENOMEM);

	/* init sigmasks */
	SIGEMPTYSET(_thread_sig_pending);
	SIGEMPTYSET(_thread_sig_blocked);
	SIGEMPTYSET(_thread_proc_sig_blocked);

	SIGEMPTYSET(_threadspec_sigmask);
	SIGADDSET(_threadspec_sigmask, SIGBUS);
	SIGADDSET(_threadspec_sigmask, SIGEMT);
	SIGADDSET(_threadspec_sigmask, SIGFPE);
	SIGADDSET(_threadspec_sigmask, SIGILL);
	SIGADDSET(_threadspec_sigmask, SIGPIPE);
	SIGADDSET(_threadspec_sigmask, SIGSEGV);
	SIGADDSET(_threadspec_sigmask, SIGSYS);

	SIGEMPTYSET(_thread_cantmask);
	SIGADDSET(_thread_cantmask, SIGKILL);
	SIGADDSET(_thread_cantmask, SIGSTOP);

	/* Ensure that SIGALRM is not masked so pthreads work properly */
	SIGEMPTYSET(_proc_sig_blocked);
	SIGADDSET(_proc_sig_blocked, SIGALRM);
	SIGADDSET(_proc_sig_blocked, SIGTRAP); /* for debuggers */
	_syscall_sys_sigprocmask(SIG_UNBLOCK, &_proc_sig_blocked,
	   &_thread_proc_sig_blocked_orig);
	_thread_proc_sig_blocked = _thread_proc_sig_blocked_orig;

	/*
	 * _thread_proc_sig_blocked should now be set to the sigmask of the
	 * process.
	 */
	SIGDELSET(_thread_proc_sig_blocked, SIGALRM);

	/* Loop thru all signals to get the existing user signal handling
	 * info
	 */
	for (sig = 1; sig < NSIG; sig++) {
		(void) _syscall_sys_sigaction(sig, NULL,
		   &sigvector[IDX(sig)]);
		_thread_sigcount[sig] = 0;
	}

	/* Initialise the global signal action structure */
	SIGFILLSET(act.sa_mask);
	SIGDELSET(act.sa_mask, SIGTRAP);	/* For debuggers */
	act.sa_flags = SA_RESTART;
	act.sa_handler = (void (*)()) _thread_sig_handler;

	/* 
	 * Loop through all signals checking for installed user handlers.
	 * Whenever a user handler is found install ours instead. We have
	 * previously saved the user's handler information.
	 * Don't try to install handlers for signals which can't be caught.
	 * ALWAYS install handlers for SIGALRM and SIGCHLD.
	 */
	for (sig = 1; sig < NSIG; sig++) {
		/* Check for signals which cannot be trapped: */
		if (SIGISMEMBER(_thread_cantmask, sig))
			continue;

		if ((sig != SIGALRM) && (sig != SIGCHLD) && 
		    ((sigvector[IDX(sig)].sa_handler == SIG_IGN) || 
		     (sigvector[IDX(sig)].sa_handler == SIG_DFL)))
			continue;

		TR("_thread_sig_init_sigaction", sig, act.sa_handler);
		(void) _syscall_sys_sigaction(sig, &act, NULL);
	}

	return (0);
}

/* 
 * Block delivery of all signals at the process level.
 * This is used to protect the few variables that may
 * be modified at signal delivery time.
 */
void
_thread_sig_block(void)
{
	sigset_t allsig;

	TR("_thread_sig_block", 0, 0);

	SIGFILLSET(allsig);
	SIGDELSET(allsig, SIGTRAP);
	_syscall_sys_sigprocmask(SIG_SETMASK, &allsig,
	   &_thread_proc_sig_blocked);

	TR("_thread_sig_block oldmask", 0, 0);
}

/*
 * Clear the process level signal mask completely allowing
 * delivery of any blocked signals.  This is used in 
 * conjunction with _thread_sig_block() to create critical
 * sections around variables managed in the signal handler.
 */
void
_thread_sig_unblock(void)
{

	TR("_thread_sig_unblock", 0, 0);

	_syscall_sys_sigprocmask(SIG_SETMASK, &_thread_proc_sig_blocked, NULL);
}

/*
 * Global signal handler for the thread package.
 *
 * This routine will handle signals for all the threads and
 * direct them as necessary to the entire process or to an
 * individual thread.
 *
 * This routine will check the state of the kernel monitor
 * and act accordingly.  In particular if the monitor isn't
 * currently locked we'll deliver the signal now if possible
 * and enter the dispatcher, which may cause a reschedule.
 */
void
_thread_sig_handler(sig, code, scp)
	int sig, code;
	struct sigcontext *scp;
{
	sigset_t stmp;

	TR("_thread_sig_handler", sig, code);
	TR("_thread_sig_handler", scp, 0);

	/* 
	 * If the kernel is locked we won't handle it now.
	 * Remember the signal to force a trip through the 
	 * dispatcher when the kernel monitor is exited.
	 */
	if (_thread_kern_locked()) {
		TR("_thread_sig_handler_kern_locked", sig, 0);
		SIGADDSET(_thread_sig_blocked, sig);
		return;
	}

	SIGEMPTYSET(stmp);
	SIGADDSET(stmp, sig);

	/* 
	 * Lock the thread kernel and unblock signals.
	 * Anything new that arrives will get handled above.
	 */
	_thread_kern_enter();
	_thread_sig_unblock();
	_thread_sig_deliver_pending(stmp, code, scp, 1);
	_thread_kern_exit();
	TR("_thread_sig_handler_end", 0, 0);
	return;
}

/*
 * Wakeup a thread blocked in a state that should be interrupted when
 * a signal arrives.  This includes POSIX.1 calls where SA_RESTART isn't
 * set on the signal type and select.  Also sigpause.
 *
 * Assumes the pthread kernel is locked upon entry and leaves it
 * locked on exit.
 *
 * Returns zero if the thread could not be awakened for any reason, non-zero
 * otherwise.
 */
int
_thread_sig_wakeup(sig, threadp)
	int     sig;
	struct pthread *threadp;
{
	int found = 0;

	TR("_thread_sig_wakeup", sig, threadp);

	if (threadp == NULL)
		return (0);

	switch (threadp->state) {
	/* 
	 * States for which there is nothing to do or where 
	 * signals have no effect.
	 */
	case PS_RUNNING:
	case PS_RUNNABLE:
	case PS_SUSPENDED:
		break;

	/* 
	 * States that are interrupted by the occurrence of a
	 * signal other than the scheduling alarm: 
	 */
	case PS_IO_WAIT:
	case PS_SLEEP_WAIT:
		if (SIGISMEMBER(threadp->sigmask, sig))
			break;
		/* XXX - Is this right?  We won't run the signal
		   handler */
		if (sigvector[IDX(sig)].sa_flags & SA_RESTART)
			break;
		/* Fall through */

	case PS_SELECT_WAIT:
	case PS_MUTEX_WAIT:	/* XXX - Process, pthread_mutex_lock will continue waiting */
	case PS_JOIN_WAIT:	/* XXX - Process, pthread_join() will continue waiting */
	case PS_WAIT_WAIT:	/* XXX - Make sure INTERRUPTED was set */
	case PS_CLOSE_WAIT:	/* XXX - Internal code needs to loop */
		if (SIGISMEMBER(threadp->sigmask, sig))
			break;
		TR("_thread_sig_wakeup_one", sig, threadp);
		_thread_seterrno(threadp, EINTR);
		_thread_kern_setrunnable(threadp);
		found++;
		break;

	case PS_COND_WAIT:
		TR("_thread_sig_wakeup_PS_COND_WAIT", sig, threadp);
		_thread_kern_setrunnable(threadp);
		found++;
		break;

	/* 
	 * Handle sigpause/sigsuspended threads 
	 * only one threads gets the signal
	 */
	case PS_SIG_WAIT:
		TR("_thread_sig_wakeup_PS_SIG_WAIT", sig,
		    threadp);
		threadp->flags |= PTHREAD_INTERRUPTED;
		_thread_seterrno(threadp, EINTR);
		_thread_kern_setrunnable(threadp);
		found++;
		break;

	case PS_DEAD:
		break;
	}

	TR("_thread_sig_wakeup_exit", sig, found);
 	return (found);
}
 
 
/*
 * Wakeup at most one thread blocked in a state that should be interrupted
 * when a signal arrives.  This includes POSIX.1 calls where SA_RESTART isn't
 * set on the signal type and select.  Also sigpause. Searches all threads
 * until it finds one.
 *
 * Assumes the pthread kernel is locked upon entry and leaves it
 * locked on exit.
 */
void
_thread_sig_wakeup_one(sig)
	int     sig;
{
	struct pthread *threadp;
 
	TR("_thread_sig_wakeup_one", sig, 0);
 
	/*
	 * Traverse the thread link list looking for threads that are blocked
	 * in an interruptible wait for which this signal is not masked. 
	 */
	for (threadp = _thread_allthreads; threadp != NULL;
	   threadp = threadp->nxt)
		if (_thread_sig_wakeup(sig, threadp))
			break;
 
	TR("_thread_sig_wakeup_one_end", sig, 0);
	return;
}

/*
 * Post a signal to a specific thread or to the process as appropriate.
 * Assumes the thread monitor is locked and leaves it that way.
 */
int
_thread_sig_post(threadp, sig)
	struct pthread * threadp;
	int sig;
{
	int action;
	struct pthread * selfp = _thread_selfp();

	TR("_thread_sig_post", threadp, sig);

	/* For compatability with kill and pthread_kill */
	if (sig == 0)
		return (0);

	/* Check if the signal number is out of range: */
	if ((sig < 1) || (sig > NSIG))
		return (EINVAL);

	action = (int) sigvector[IDX(sig)].sa_handler;
	/* check for default actions - if found let the kernel deliver it */
	if (action == (int) SIG_DFL || action == (int) SIG_IGN) {
		kill(getpid(), sig);
		return (0);
	}

	/* 
	 * If thread pointer is not valid, post the signal to the
	 * process and let the first capable thread handle it.
	 */
	if (threadp == NULL) {
		SIGADDSET(_thread_sig_pending, sig);
		return (0);
	}

	/*
	 * Post the signal to specified thread.
	 */
	TR("_thread_sig_post_state", threadp, threadp->state);

	switch (threadp->state) {

	case PS_RUNNABLE:
		SIGADDSET(threadp->sigpend, sig);
		if (SIGISMEMBER(threadp->sigmask, sig))
			return (0);
		threadp->flags |= PTHREAD_INTERRUPTED;
		
		if (_thread_queue_remove(&_thread_runq, threadp) == 0)
			_thread_queue_enq(&_thread_runq, threadp);

		switch (selfp->schedpolicy) {
		case SCHED_FIFO:
			break;
		case SCHED_RR:
		case SCHED_OTHER:
			/* 
			 * If the time quantum has expired (or
			 * was never set if it was the only
			 * thread) then force a reschedule on
			 * exit of thread monitor.
			 */
			if (!timespecisset(&selfp->timer))
				_thread_kern_need_resched = 1;
			break;
		default:
			abort();
		}

		/* 
		 * If the new thread is higher priority,
		 * force a reschedule.
		 */
		if (selfp->priority < threadp->priority)
			_thread_kern_need_resched = 1;
		break;

	/* 
	 * States for which there is nothing to do or where 
	 * signals have no effect.
	 */
	case PS_RUNNING:
	case PS_SUSPENDED:
		SIGADDSET(threadp->sigpend, sig);
		if (SIGISMEMBER(threadp->sigmask, sig))
			break;
		threadp->flags |= PTHREAD_INTERRUPTED;
		break;

	/* 
	 * States that are interrupted by the occurrence of a
	 * signal other than the scheduling alarm: 
	 */
	case PS_IO_WAIT:
	case PS_SLEEP_WAIT:
		SIGADDSET(threadp->sigpend, sig);
		if (SIGISMEMBER(threadp->sigmask, sig))
			break;
		threadp->flags |= PTHREAD_INTERRUPTED;
		if (sigvector[IDX(sig)].sa_flags & SA_RESTART)
			break;
		/* Fall Through */

	case PS_CLOSE_WAIT:
	case PS_COND_WAIT:
	case PS_JOIN_WAIT:
	case PS_WAIT_WAIT:
	case PS_MUTEX_WAIT:
		/* XXX - is this right? */
	case PS_SELECT_WAIT:
		SIGADDSET(threadp->sigpend, sig);
		if (SIGISMEMBER(threadp->sigmask, sig))
			break;
		/* Fall through */

	case PS_SIG_WAIT:
		/* Add this signal to the pending set for this thread */
		SIGADDSET(threadp->sigpend, sig);
		threadp->flags |= PTHREAD_INTERRUPTED;

		/* Set return code and make this thread runnable runnable */
		_thread_seterrno(threadp, EINTR);
		_thread_kern_setrunnable(threadp);
		break;

	case PS_DEAD:
	default:
		break;
	}

	return (0);
}

/*
 * Deliver any pending signals that are not masked at the
 * thread or process level.
 * There is only one case where code and scp are not 0 (NULL) and that is
 * the case where we get called from the thread signal handler. In that case
 * both code and scp will be valid and nonzero and can be used by the
 * the user's signal handler if it gets called. Also in this case one and
 * only one signal is set in blocked.
 *
 * MUST be called with the thread kernel locked.
 */
void
_thread_sig_deliver_pending(blocked, code, scp, exec_user)
	sigset_t blocked;
	int code;
	struct sigcontext *scp;
	int exec_user;
{
	int sig;
	sigset_t signals;
	struct pthread *selfp;
	int icode = code;
	struct sigcontext *iscp = scp;
	sigset_t stmp;

	selfp = _thread_selfp();

	TR("_thread_sig_deliver_pending", selfp, 0);

	if (SIGNOTEMPTY(blocked)) {
		if (SIGISMEMBER(blocked, SIGALRM)) {
			THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD,
			    EVENT_THREAD_SIG_DELIVER, SIGALRM);
			SIGDELSET(blocked, SIGALRM);
			_thread_timer_dotimeouts(SIGALRM);
		}
		TR("_thread_sig_deliver_pending1", blocked, 0);
		if (SIGISMEMBER(blocked, SIGCHLD)) {
			THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD,
			    EVENT_THREAD_SIG_DELIVER, SIGCHLD);
			/* Wakeup anyone in WAIT_WAIT state. */
			while (_thread_kern_wakeup(&_thread_waitq) != NULL)
				continue;

			/* If there is no user handler clear it now */
			if ((sigvector[IDX(SIGCHLD)].sa_handler ==
			   SIG_DFL) ||
			   (sigvector[IDX(SIGCHLD)].sa_handler ==
			   SIG_IGN))
				SIGDELSET(blocked, SIGCHLD);
		}
	}

	signals = blocked;
	SIGSETOR(signals, _thread_sig_pending); 
	SIGSETOR(signals, selfp->sigpend);

	TR("_thread_sig_deliver_pending2", 0, 0);

	if (SIGISEMPTY(signals))
		return;

	THREAD_EVT_OBJ_USER_0(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_SIG_DELIVER);
	TR("_thread_sig_deliver_pending3", 0, 0);

	/* 
	 * Loop through the signals delivering them if possible.
	 * signals that fail are left in the mask. If we deliver one
	 * we'll clear it.
	 * If a signal which is NOT directed to a particular thread is
	 * undeliverable then wakeup all threads which are waiting and do
	 * NOT have this signal masked.
	 */
	for (sig = 1; sig < NSIG; sig++) {
		if (!SIGISMEMBER(signals, sig))
			continue;

		if (SIGISMEMBER(blocked, sig)) {
			icode = code;
			iscp = scp;
			code = 0;
			scp = NULL;
		} else {
			icode = 0;
			iscp = NULL;
		}
		if (exec_user) {
			SIGDELSET(selfp->sigpend, sig);
			SIGDELSET(_thread_sig_pending, sig);
		}
		if (_thread_sig_deliver_one(sig, icode, iscp, exec_user) == 0) {
			if (exec_user)
				SIGDELSET(signals, sig);
		} else {
			if (SIGISMEMBER(blocked, sig) || 
			   SIGISMEMBER(_thread_sig_pending, sig))
				_thread_sig_wakeup_one(sig);
		}
	}
	
	stmp = signals;
	SIGSETAND(stmp, _threadspec_sigmask);
	SIGSETOR(selfp->sigpend, stmp);

	_thread_sig_pending = signals;
	SIGSETANDC(_thread_sig_pending, selfp->sigpend);

	TR("_thread_sig_deliver_pending_end", 0, 0);
	return;
}

/*
 * Try to deliver a single signal.  This does the real work
 * of signal delivery. If a signal is undeliverable (blocked)
 * return EAGAIN, else do the work and return 0. We set the
 * per-thread signal mask to that defined by the sigaction
 * structure during delivery of this signal.
 *
 * Called with the thread kernel monitor locked and it leaves
 * it locked on exit.
 */
typedef int (*ACTPTR)(int, int, struct sigcontext *);

static int
_thread_sig_deliver_one(sig, code, scp, exec_user)
	int sig, code;
	struct sigcontext *scp;
	int exec_user;
{
	ACTPTR action;
	sigset_t tmask, newsig;
	struct pthread *selfp = _thread_selfp();

	TR("_thread_sig_deliver_one", sig, selfp);

	if (selfp->state == PS_DEAD || SIGISMEMBER(selfp->sigmask, sig)) {
		TR("_thread_sig_deliver_one_EAGAIN", sig, selfp->sigmask);
		return (EAGAIN);
	}
	(void) _thread_sig_wakeup(sig, selfp);

	action = (ACTPTR) sigvector[IDX(sig)].sa_handler;
	/* 
	 * Check for default actions. We should not get here as default
	 * actions should cause the signal to be sent to the kernel for
	 * delivery by thread_sig_post().
	 */
	if (action == (ACTPTR) SIG_DFL || action == (ACTPTR) SIG_IGN) {
		TR("_thread_sig_deliver_one_DEFAULT", action, 0);
		kill(getpid(), sig);
		return (0);
	}

	if (action == (ACTPTR) SIG_ERR) {
		TR("_thread_sig_deliver_one_SIG_ERR", action, selfp->state);
		return (EAGAIN);
	}

	newsig = sigvector[IDX(sig)].sa_mask;
	SIGADDSET(newsig, sig);
	_thread_sigmask(SIG_BLOCK, &newsig, &tmask);

	/* 
	 * we MUST release the kernel lock before calling any user functions.
	 * However, if the user calls kernel functions then the lock will
	 * be set and reset. _thread_kernel_exit() checks for the need to
	 * reschedule and can suspend this thread (in the middle of signal
	 * handling) should this happen. Clear the _thread_kern_need_resched
	 * flag before entering the user's function to avoid this. Restore
	 * it and relock after the user function has completed.
	 */
	TR("_thread_sig_deliver_one_action", selfp, action);

	/* From _thread_kern_switch(), we might not want to run user handler */
	if (exec_user) {
		_thread_kern_exit_always();
		/* call the user's signal handler. Code and scp may be zero */
		action(sig, code, scp);
		_thread_kern_enter();
	}

	selfp->flags |= PTHREAD_INTERRUPTED;
	_thread_seterrno(selfp, EINTR);

	_thread_sigmask(SIG_SETMASK, &tmask, 0);
	TR("_thread_sig_deliver_one_action_end", selfp, action);

	return (0);
}

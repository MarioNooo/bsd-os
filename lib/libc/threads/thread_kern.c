/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_kern.c,v 1.26 2003/10/01 17:19:39 giff Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

/* Forward References */
#ifdef DEBUG
void	_thread_kern_check_runq __P((void));
#endif
static void timeout_handler __P((void * arg));
static void set_resched_timer __P((struct pthread *));

/* Global data structures */

struct pthread * _thread_allthreads;	/* List of all threads in the system */
struct pthread * _thread_deadthreads;	/* Threads awaiting cleanup */

#if WV_INST
thread_evtlog_t _thread_evtlog;
#else
struct pthread * _thread_run = NULL;	/* XXX Single processor only */
#endif

struct pthread_queue _thread_runq;
struct pthread_queue _thread_waitq;	/* For threads blocked in wait() */
struct pthread * _thread_kern_owner;
int _thread_kern_need_resched;
extern int _thread_count;	/* count of active threads */
int thread_sched_usecs;

int
_thread_kern_init()
{
	char *quantum_s;

	TR("_thread_kern_init", 0, 0);

	_thread_kern_need_resched = 0;
	_thread_kern_owner = NULL; 	/* Who owns the kern lock */
	_thread_queue_init(&_thread_runq);
	_thread_queue_init(&_thread_waitq);
	_thread_list_init(&_thread_allthreads);
	_thread_list_init(&_thread_deadthreads);
	_thread_run = NULL;
	_thread_count = 0;

	/* Get scheduling quantum from user if supplied */
	quantum_s = getenv("THREAD_SCHED_USECS");

	/* Round to 10 ms increment */
	thread_sched_usecs = (quantum_s == NULL) ? 0 :
	    10000 * ((int) ((atoi(quantum_s) + 5000) / 10000));

	/* Take default value if out of range of 10ms .. 500ms */
	if ((thread_sched_usecs < 10000) || (thread_sched_usecs > 500000))
		thread_sched_usecs = THREAD_SCHED_USECS;

	return (0);
}

/*
 * Function versions of the kernel monitor macros.  These are just for
 * special case applications that need to lock the monitor from outside
 * the pthreads proper.  If you call this you BETTER KNOW WHAT YOU ARE
 * DOING.
 */

void 
_thread_kern_sched_lock()
{

	TR("_thread_kern_sched_lock", 0, 0);
	_thread_kern_enter();
}

void
_thread_kern_sched_unlock()
{

	TR("_thread_kern_sched_unlock", 0, 0);
	_thread_kern_exit();
}

int
_thread_kern_sched_locked()
{

	TR("_thread_kern_sched_locked", 0, 0);
	return (_thread_kern_locked());
}

/*
 * A threads scheduling quantum expired.  Set the resched event and
 * clear the time remaining in the pthread timer field.
 */
static void
resched_timeout(arg)
	void * arg;
{

	TR("resched_timeout", arg, 0);
	_thread_kern_need_resched = 1;
}

static void
set_resched_timer(threadp)
	struct pthread *threadp;
{
	struct timespec ts;

	TR("set_resched_timer", threadp, 0);

	/* Timeout is in range of 10ms to 500ms in 10ms increments */
	ts.tv_sec = THREAD_SCHED_SECS;
	ts.tv_nsec = (thread_sched_usecs * 1000);
	_thread_timer_timeout(resched_timeout, threadp, &ts);
}

/*
 * The pthread dispatcher.
 *
 * Handles context switching for user level pthreads.
 * Save the state of the currently executing thread.  
 * To context switch we call a machine dependent routine
 * to save the current context.  Savectx returns non-zero 
 * from a restorectx (similar to setjmp/longjmp) so if we 
 * are returning from a restore we return.  
 *
 * The kernel remains locked throughout the dispatcher.
 *
 * When a thread has its context restored it begins execution immediately
 * following the last savectx that it did. This means that any work that it
 * does following the savectx but BEFORE the restctx which loads the new
 * thread context is wasted work. EXTREME care should be taken, however,
 * to ensure that execution flows from the savectx to restctx as fast as
 * possible. ANY interruption in this area may cause the thread to become
 * lost.
 *
 */
void
_thread_kern_switch(wantsig)
	enum switch_signal wantsig;
{
	struct pthread *selfp, *threadp;
	int canblock, sig;
	sigset_t sigs;
	sigset_t pending;

	selfp = _thread_selfp();

	TR("_thread_kern_switch", selfp, _thread_kern_owner);

	/*
	 * Loop looking for a thread to run.  If there are none we'll block
	 * in a select below and then retry again.  
	 */
	threadp = selfp;
	SIGEMPTYSET(sigs);
	canblock = 0;
	while (1) {
		/*
		 * If any signals/events happened while we are in the
		 * dispatcher we'll notice and handle them here.
		 */
		if (SIGNOTEMPTY(_thread_sig_blocked)) {
			TR("_thread_kern_switch_sig_blocked", 0, 0);

			/* Block signal delivery while we get and clear sigs */
			_thread_sig_block();
			sigs = _thread_sig_blocked;
			SIGEMPTYSET(_thread_sig_blocked);
			_thread_sig_unblock();

			/* Deliver signals, possibly waking someone */
			_thread_sig_deliver_pending(sigs, 0, 0,
			    (wantsig == SWITCH_SIGNALS) ? 1 : 0);
		}
		/*
		 * Poll for I/O, possibly waking threads.  A signal could
		 * arrive during its select() call and we should ignore
		 * EINTR [Python regression tests caught this].
		 */
		_thread_aio_poll(canblock);
		if (errno == EINTR)
			errno = 0;
		canblock = 0;

		if (_thread_kern_need_resched != 0) {
			TR("_thread_kern_switch_resched", selfp, 0);
			threadp = _thread_queue_get(&_thread_runq);
			if ((threadp == NULL) && (selfp->state == PS_RUNNING))
				threadp = selfp;
			else {
				selfp->error = errno;	/* XXX */
				if (selfp->state == PS_RUNNING) {
					selfp->state = PS_RUNNABLE;
					if (selfp->queue != NULL)
						_thread_queue_remove(
						    selfp->queue, selfp);
					_thread_queue_enq(&_thread_runq, selfp);
				}
				if (_thread_machdep_savectx(
				   &selfp->saved_context) == 1) {
					/* Resuming with restored context */

					selfp = _thread_selfp();
					TR("_thread_kern_new_thread",
					   selfp, threadp);
					threadp = NULL;
					if (selfp->can_state ==
					    PCS_ASYNCHRONOUS) {
						TR("_thread_kern_new_cancel",
						    selfp, 0);
						_thread_cancel();
					}
					goto outahere;
				}
			}

			/* 
		 	 * If there's no runnable thread block in an I/O poll
		 	 * with signals unmasked.  If there are no threads
		 	 * at all, (i.e. _thread_allthreads is null) just exit.
		 	 */
			if (threadp == NULL) {
				if (_thread_allthreads == NULL) {
					_thread_kern_owner = NULL;
					_thread_kern_need_resched = 0;
					exit(0);
				}

				threadp = selfp;
				canblock = 1;

				continue;
			}
			_thread_kern_need_resched = 0;
		}

		/* remove the current timer if its still pending */
		_thread_timer_untimeout(resched_timeout, selfp, NULL);

		if (selfp != threadp) {
			if (_thread_queue_remove(&_thread_runq, threadp) < 0)
				abort();
			errno = threadp->error;		/* XXX */
			TR("_thread_kern_switch_error2", threadp, errno);

			_thread_run = threadp;		/* XXX */
		}

		TR("_thread_kern_switch_sched", selfp, threadp);

		_thread_kern_owner = threadp;

		_thread_kern_sched_state(threadp, PS_RUNNING);

		switch (threadp->schedpolicy) {
		case SCHED_FIFO:
			break;
		case SCHED_RR:
		case SCHED_OTHER:
			/* if this is round robin scheduled set a timer */
			set_resched_timer(threadp);
			break;
		default:
			abort();
		}

		if (threadp == selfp)
			goto outahere;

		/* Restore the saved context and run on the switched thread */
		_thread_machdep_restctx(&threadp->saved_context, 1);

		/* NOTREACHED */
	}

outahere:
	/*
	 * There is a period of time here immediately following the point at
	 * which we unblock signals but before we unlock the kernel where
	 * signals may be received by the thread kernel but not acted upon.
	 * Signals which arrive during this period will be processed upon the
	 * next entry into the thread kernel.
	 */
	TR("_thread_kern_switch_outahere", selfp, threadp);
	if (wantsig == SWITCH_SIGNALS) {
		_thread_sig_block();
		sigs = _thread_sig_blocked;
		SIGEMPTYSET(_thread_sig_blocked);
		_thread_sig_unblock();
		/* Deliver any signals we can on the new thread */
		_thread_sig_deliver_pending(sigs, 0, 0, 1);
		SIGEMPTYSET(sigs);
	}

#if 0
	_thread_kern_check_runq();
#endif
	TR("_thread_kern_switch_end", selfp, threadp);

	return;
}

/*
 * This routine needs to do some special handling since it represents
 * and "alternate" exit point from the thread kernel.  When a thread
 * is initially created the first time its made runnable we'll come here.
 */
void
_thread_kern_start(void)
{
	struct pthread *selfp = _thread_selfp();

	TR("_thread_kern_start", selfp, 0);

	/* 
	 * Unlock the kernel
	 * Normally the kernel is unlocked on the way out of
	 * thread_kern_switch(), however this is the first time this thread
	 * has run and so savectx takes us here instead of thread_kern_switch().
	 */
	_thread_kern_owner = NULL;
	
	pthread_exit(selfp->start_routine(selfp->arg));
	/* NOTREACHED */
}

/*
 * Clean up memory resources for threads that have exited and that are not
 * zombied (i.e. either detached or exit status has been retrieved. We
 * gotta be carefull here.  We may be running on the stack of a thread
 * thats eligible for cleanup.  We'll save it for later since having your
 * stack go away underneath you is an unhappy event.
 */
void
_thread_kern_cleanup_dead()
{
	struct pthread * selfp;
	struct pthread * freelistp;
	struct pthread * indexp;
	struct pthread * threadp;

	freelistp = NULL;
	selfp = _thread_selfp();

	TR("_thread_kern_cleanup_dead", selfp, 0);

	/* Process the head of the list */
	while (((indexp = _thread_deadthreads) != NULL)  &&
	       ((indexp->flags & PTHREAD_ZOMBIE) == 0)) {
		/* Don't cleanup an active stack */
		if (indexp == selfp)
			break;
		_thread_deadthreads = indexp->nxt;
		_thread_list_add(&freelistp, indexp);
	}

	while (indexp && ((threadp = indexp->nxt) != NULL)) {
		if ((threadp == selfp) ||
		    ((threadp->flags & PTHREAD_ZOMBIE) != 0))
			indexp = threadp;
		else {
			indexp->nxt = threadp->nxt;
			_thread_list_add(&freelistp, threadp);
		}
	}

	/* Process the free list we built, freeing up resources */
	while ((indexp = freelistp) != NULL) {
		freelistp = indexp->nxt;
		if (indexp->attr.stackaddr == NULL)
			_thread_machdep_stack_free(indexp->stacksize, 
				indexp->stackaddr);
		free(indexp);
	}

	return;
}

/*
 * After a fork, insure that the only thread present in the child is
 * the one that did the fork.
 */
void
_thread_kern_cleanup_child()
{
	struct pthread *selfp;
	struct pthread *indexp;

	selfp = _thread_selfp();

	TR("_thread_kern_cleanup_child", selfp, 0);

	/* Nobody to reschedule to */
	_thread_kern_need_resched = 0;

	/* 
	 * Remove all thread from the allthreads list, put all but our
	 * thread onto the dead thread list.
	 */
	while ((indexp = _thread_allthreads) != NULL) {
		_thread_list_delete(&_thread_allthreads, indexp);
		if (indexp != selfp)
			_thread_list_add(&_thread_deadthreads, indexp);
	}
	/* Add us back to the allthreads list */
	_thread_list_add(&_thread_allthreads, selfp);

	/* Destroy all the threads on the deadthreads list */
	while ((indexp = _thread_deadthreads) != NULL) {
		_thread_deadthreads = indexp->nxt;
		/* 
		 * Remove any events pending for this thread.  If
		 * events are associated with a thread, their arg will
		 * be the threadp.
		 */
		while (_thread_timer_untimeout(NULL, indexp, NULL) == 0)
		    ;
		/* Remove from any queues */
		if (indexp->queue)
			_thread_queue_remove(indexp->queue, indexp);
		/* Free the stack */
		if (indexp->attr.stackaddr == NULL)
			_thread_machdep_stack_free(indexp->stacksize, 
				indexp->stackaddr);
		/* And finally the pthread structure itself */
		free(indexp);
	}

	TR("_thread_kern_cleanup_child done", selfp, 0);
}

/*
 * Timeout handler for thread_block/wakeup.
 */
static void 
timeout_handler(arg)
	void * arg;
{
	struct pthread * threadp = arg;

	TR("timeout_handler", arg, 0);

	threadp->flags |= PTHREAD_TIMEDOUT;
	threadp->timer.tv_sec = 0;
	threadp->timer.tv_nsec = 0;

	/*
	 * If the thread is currently blocked on a queue
	 * Remove it and make it runnable.
	 */
	if (threadp->queue)
		_thread_queue_remove(threadp->queue, threadp);

	if (threadp->state != PS_SUSPENDED && threadp->state != PS_DEAD)
		_thread_kern_setrunnable(threadp);

#if 0
	if ((_thread_queue_member(&_thread_runq, threadp) == 0) &&
	   threadp != _thread_selfp())
		abort();
#endif
}

/*
 * Block the calling thread on the "queue".  The scheduling state
 * of the thread is set to "state."
 *
 * Assumes the caller is the thread that is to be blocked and that the
 * scheduling kernel is locked upon entry. 
 * If timeout is non null it is used to initialize the per thread timer.
 * Upon wakeup this timer will contain the time remaining on the timer
 * or zero if the block timed out and the scheduler lock is unlocked.
 *
 * Returns ETIMEDOUT if a timeout expired.
 * Returns EINTR if an I/O operation was interrupted.
 */
int
_thread_kern_block(queue, state, timeout)
	struct pthread_queue * queue;
	enum pthread_state state;
	struct timespec * timeout;
{
	struct pthread * selfp;

	TR("_thread_kern_block", queue, state);
	if (timeout)
		TR("_thread_kern_block", timeout->tv_sec, timeout->tv_nsec);

	/* Check for cancellation */
	switch (state) {
	case PS_COND_WAIT:
	case PS_JOIN_WAIT:
	case PS_WAIT_WAIT:
	case PS_SIG_WAIT:
		/* In these states we just go away */
		_thread_testcancel();
		break;

	case PS_IO_WAIT:
	case PS_SELECT_WAIT:
		/* In this state we need to know we are being canceled */
		if (_THREAD_TESTCANCEL())
			return (ETHREAD_CANCELED);
		break;

	case PS_MUTEX_WAIT:
	case PS_CLOSE_WAIT:
		/* We ignore cancellation here */
		break;
	}

	selfp = _thread_selfp();
	_thread_kern_sched_state(selfp, state);

	if (queue != NULL) {
		if (selfp->queue != NULL)
			_thread_queue_remove(selfp->queue, selfp);
		_thread_queue_enq(queue, selfp);
	}

	selfp->flags &= ~(PTHREAD_TIMEDOUT | PTHREAD_INTERRUPTED);

	if (timeout) {
		selfp->timer = *timeout;
		_thread_timer_timeout(timeout_handler, selfp, &selfp->timer);
	} else {
		selfp->timer.tv_sec = 0;
		selfp->timer.tv_nsec = 0;
	}

	_thread_kern_need_resched = 1;
	_thread_kern_switch(SWITCH_NOSIGNALS);

	/* Back from the blocking operation */
	if ((selfp->flags & PTHREAD_TIMEDOUT) != 0)
		return (ETIMEDOUT);
	else if ((selfp->flags & PTHREAD_INTERRUPTED) != 0)
		return (EINTR);
	else
		return (0);
}

/*
 * Dequeue a waiting thread and make it runnable.  
 * We'll un-schedule any timeout posted and return the
 * timeleft in the per-thread timer.
 */
struct pthread *
_thread_kern_wakeup(queue)
	struct pthread_queue * queue;
{
	struct pthread * threadp;

	TR("_thread_kern_wakeup", queue, 0);

	if ((threadp = _thread_queue_deq(queue)) != NULL) {
		TR("_thread_kern_wakeup1", queue, threadp);
		/* Clear any timeout that is pending */
		_thread_timer_untimeout(timeout_handler, threadp, 
				&threadp->timer);
		_thread_kern_setrunnable(threadp);
	}


	return (threadp);
}

/*
 * Sets the thread referenced by "threadp" to a runnable state.
 * If the scheduling policy is pre-emptive and the new thread
 * is higher priority than the current thread then set a flag
 * indicating we need a reschedule.
 *
 * If the suspended flag is set we won't become runnable but
 * instead enter a suspended state.
 *
 * There is a tricky bit to this.  If the thread we are going to
 * make runnable was blocked and no other thread was available to
 * run we'll use its stack to block.  If thats the case we are 
 * already "runnable" we just need to set our state to running.
 */
void 
_thread_kern_setrunnable(threadp)
	struct pthread * threadp;
{
	struct pthread * selfp;

	selfp = _thread_selfp();

	TR("_thread_kern_setrunnable", threadp, selfp);

	if (threadp->flags & PTHREAD_SUSPEND_PENDING) {
		threadp->flags &=  ~(PTHREAD_SUSPEND_PENDING);
		_thread_kern_sched_state(threadp, PS_SUSPENDED);
	} else if (threadp == selfp) 
		_thread_kern_sched_state(threadp, PS_RUNNING);
	else {
		_thread_kern_sched_state(threadp, PS_RUNNABLE);
		if (threadp->queue != NULL)
			_thread_queue_remove(threadp->queue, threadp);
		_thread_queue_enq(&_thread_runq, threadp);

		switch (selfp->schedpolicy) {
		case SCHED_FIFO:
			break;
		case SCHED_RR:
		case SCHED_OTHER:
			/* 
			 * If the time quantum has expired (or was
			 * never set if it was the only thread) then
			 * force a reschedule on exit of thread
			 * monitor.
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
	}

	return;
}

#if 0
void _thread_kern_check_runq()
{
	struct pthread *threadp;

	TR("_thread_kern_check_runq", 0, 0);

	for (threadp = _thread_allthreads; threadp != NULL;
	   threadp = threadp->nxt) {
		if (threadp->state == PS_RUNNING ||
		   threadp->state == PS_RUNNABLE)
			if ((_thread_queue_member(&_thread_runq, threadp) == 0) && (threadp != _thread_run))
				abort();
	}
}
#endif

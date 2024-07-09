/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_timer.c,v 1.11 2002/05/31 14:04:01 ericl Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include "thread_private.h"
#include "thread_trace.h"

/*
 * Timer management code.   This is used to provide multiplexed time
 * services for POSIX CLOCK_REALTIME clocks and pthreads based on 
 * the ITIMER_REAL clock. 
 */

typedef struct event_t {
	struct event_t	*next;
	void		(*func) __P((void *));
	void		*arg;
	struct timespec	when;
} event_t;

static event_t *_thread_events = 0;
static event_t * _thread_eventfree = 0;		/* Free List */

#define	event_free(e) { 			\
	(e)->next = _thread_eventfree;			\
	_thread_eventfree = (e);			\
	}

#define	NEVENTS	64			

static pthread_once_t _thread_event_init = PTHREAD_ONCE_INIT;
static int more_events();

/*
 * Initialize the timer module.  Currently this routine simply
 * allocates free list memory.
 */

static 
void _thread_timer_init()
{

	TR("_thread_timer_init", 0, 0);

	(void) more_events();
	return;
}

static int
more_events()
{
	int i;
	event_t * e;

	TR("more_events", 0, 0);

        if ((e = calloc(NEVENTS, sizeof(event_t))) == NULL)
		return (ENOMEM);

	/* build the freelist */
	_thread_eventfree = e;
	for (i = 1; i < NEVENTS; i++)
		e[i-1].next = &e[i];

	return (0);
}

/*
 * Allocate a timeout event and initialize it.  We'll also
 * call any pending timeouts and schedule a new interval
 * timer if necessary. More memory for event structures
 * is allocated if needed.
 *
 * This routine assumes the thread kernel is locked.
 */
int
_thread_timer_timeout(func, arg, tsp)
	void (*func)();
	void *arg;
	struct timespec *tsp;
{
	event_t *f;
	event_t *e;

	TR("_thread_timer_timeout", func, arg);
	TR("_thread_timer_timeout_time", tsp->tv_sec, tsp->tv_nsec);

	_thread_once(&_thread_event_init, _thread_timer_init);

	_thread_kern_check();

	while ((e = _thread_eventfree) == NULL) {
		if (more_events() == ENOMEM) {
			TR("_thread_timer_timeout_end1", func, arg);
			return (ENOMEM);
		}
	}

	_thread_eventfree = e->next;

	/* Remove any existing timeouts for this func,arg pair */
	_thread_timer_untimeout(func, arg, NULL);

	clock_gettime(CLOCK_REALTIME, &e->when);
	timespecadd(&e->when, tsp);
	e->arg = arg;
	e->func = func;

	if (_thread_events == NULL ||
	    timespeccmp(&e->when, &_thread_events->when, <)) {
		e->next = _thread_events;
		_thread_events = e;
		_thread_timer_dotimeouts(SIGALRM);
		TR("_thread_timer_timeout_end2", func, arg);
		return (0);
	}

	f = _thread_events;
	while (f->next && timespeccmp(&e->when, &f->next->when, >=))
		f = f->next;
	e->next = f->next;
	f->next = e;
	_thread_timer_dotimeouts(SIGALRM);
	TR("_thread_timer_timeout_end", func, arg);

	return (0);
}

/*
 * Remove an active timeout from the event list.
 * If "timeleft" is a valid pointer return the
 * time remaining until this timeout would expire.
 * There can be only one...  timeout with the same
 * func/arg pair on the list at a time.
 * 
 * If func is NULL, remove the timers with the given arg.  This is
 * assumed to be a pointer to a pthread structure.
 *
 * Called with the thread monitor locked.
 */
int
_thread_timer_untimeout(func, arg, timeleft)
	void (*func)();
	void *arg;
	struct timespec * timeleft;
{
	int erc;
	event_t *e, *found;

	TR("_thread_timer_untimeout", func, arg);

	found = NULL;
	if ((e = _thread_events) && (func == NULL || e->func == func) &&
	    (e->arg == arg)) {
		_thread_events = e->next;
		found = e;
	} else {
		while (e && (found = e->next)) {
			if ((func == NULL || found->func == func) &&
			    (found->arg == arg)) {
				e->next = found->next;
				break;
			} else
				e = found;
		}
	}

	if (found != NULL) {
		if (timeleft != NULL) {
			struct timespec timenow;

			*timeleft = found->when;
			clock_gettime(CLOCK_REALTIME, &timenow);
			timespecsub(timeleft, &timenow);
		}
		event_free(found);
		erc = 0;
	} else
		erc = EINVAL;

	return (erc);
}

/*
 * Locate and return the time remaining on the timer specified by
 * the func/arg pair.  Like untimeout but does not remove the timer 
 * from the list.
 *
 * Assumes the thread monitor is locked.
 */
int
_thread_timer_timeleft(func, arg, timeleft)
	void (*func)();
	void *arg;
	struct timespec * timeleft;
{
	int erc;
	event_t *e, *found;

	TR("_thread_timer_timeleft", func, arg);

	if (timeleft == NULL)
		return (EINVAL);

	found = NULL;
	if ((e = _thread_events) && (e->func == func) && (e->arg == arg))
		found = e;
	else {
		while (e && (found = e->next)) {
			if ((found->func == func) && (found->arg == arg))
				break;
			else
				e = found;
		}
	}

	if (found != NULL) {
		struct timespec timenow;

		*timeleft = found->when;
		clock_gettime(CLOCK_REALTIME, &timenow);
		timespecsub(timeleft, &timenow);
		erc = 0;
	} else
		erc = EINVAL;

	return (erc);
}

/*
 * Process a timeout.  Called from the signal handler or 
 * from other timeout functions as necessary.  The "sig"
 * parameter is to allow this to be called directly as 
 * a signal handler if desired.  We don't use it that way
 * for threads but I didn't want to preclude it for other
 * callers.
 *
 * Assumes the thread monitor is locked.
 * Returns non-zero if an event timedout, zero otherwise.
 */
int
_thread_timer_dotimeouts(sig)
	int sig;
{
	struct timespec timenow;
	int didevent = 0;

 	TR("_thread_timer_dotimeouts", sig, _thread_events);

	clock_gettime(CLOCK_REALTIME, &timenow);

	while (_thread_events &&
	    timespeccmp(&timenow, &_thread_events->when, >=)) {
		event_t *e = _thread_events;
		_thread_events = _thread_events->next;

		TR("_thread_timer_dotimeouts1", e->func, e->arg);

		THREAD_EVT_OBJ_USER_2(EVT_LOG_PTRACE_THREAD,
		    EVENT_THREAD_TIMEOUT, e->func, e->arg);
		(*e->func)(e->arg);
		event_free(e);
		didevent++;

		/* Time has passed, refetch the time */
		clock_gettime(CLOCK_REALTIME, &timenow);
	}

	if (_thread_events) {
		struct itimerval delta;
		struct timespec deltaspec;

		timerclear(&delta.it_interval);
		deltaspec = _thread_events->when;
		timespecsub(&deltaspec, &timenow);
		/* We need POSIX timers */
		TIMESPEC_TO_TIMEVAL(&delta.it_value, &deltaspec);
		/* If delta is less than 1 usec, sleep for 1 usec */
		if (!timerisset(&delta.it_value))
			delta.it_value.tv_usec = 1;
		TR("_thread_timer_dotimeouts_setitimer", 
		    delta.it_value.tv_sec, delta.it_value.tv_usec);
     		_syscall_sys_setitimer(ITIMER_REAL, &delta, NULL);
	}

	TR("_thread_timer_dotimeouts_end", sig, didevent);
	return (didevent);
}


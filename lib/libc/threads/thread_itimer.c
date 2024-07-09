/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_itimer.c,v 1.11 2000/08/24 15:54:29 jch Exp
 */

/*
 * Interval timer management code.   This is used to provide multiplexed 
 * time * services for POSIX CLOCK_REALTIME clocks and pthreads based on 
 * the ITIMER_REAL clock. 
 */

#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "thread_private.h"
#include "thread_trace.h"

struct posix_timer {
	int signo;
	struct pthread *threadp;
	struct itimerspec timer;
};

static struct posix_timer _itimer_real;

static void alarm_handler __P((void *));

/*
 * Handles delivery of an alarm timeout.  Defined using a structure per 
 * timer that specifies the thread that owns it and signal to deliver. 
 * The arg value is a pointer to the respective timer.  This works for 
 * the regular BSD setitimer ITIMER_REAL timer and posix timers also.
 *
 * This is called from signal level or from the dispatcher main loop
 * so the thread monitor is locked already.
 */
static void
alarm_handler(arg)
	void * arg;
{
	struct posix_timer * itimer;

	TR("_thread_alarm_handler", arg, 0);

	itimer = (struct posix_timer *)arg;

	/* Clear the current timer value */
	itimer->timer.it_value.tv_sec = 0;
	itimer->timer.it_value.tv_nsec = 0;

	/* Post the specified signal */
	_thread_sig_post(itimer->threadp, itimer->signo);

	/* If there is a reload value then re-arm this timer */
	if ((itimer->timer.it_interval.tv_sec != 0) ||
	    (itimer->timer.it_interval.tv_nsec != 0)) {
		itimer->timer.it_value = itimer->timer.it_interval;
		_thread_timer_timeout(alarm_handler, arg, &itimer->timer.it_value);
	}

	TR("_thread_alarm_handler_end", arg, 0);

	return;
}

int
_thread_sys_setitimer(how, in, out)
	int how;
	struct itimerval *in;
	struct itimerval *out;
{
	struct pthread * selfp;
	struct timespec ts;

	selfp = _thread_selfp();

	TR("_thread_sys_setitimer", selfp, how);

	if ((how == ITIMER_VIRTUAL) || (how == ITIMER_PROF))
		return (_syscall_sys_setitimer(how, in, out));

	if (how != ITIMER_REAL)
		return (EINVAL);

	if (in != NULL &&
	    timevalinvalid(&in->it_value) || timevalinvalid(&in->it_interval))
		return (EINVAL);

	_thread_kern_enter();

	/* Remove the current timeout and return time left */
	_thread_timer_untimeout(alarm_handler, (void *)&_itimer_real, &ts);

	if (out != NULL) {
		/* convert the timeleft */
		TIMESPEC_TO_TIMEVAL(&out->it_value, &ts);
		/* Copy in the reload value */
		TIMESPEC_TO_TIMEVAL(&out->it_interval,
		   &_itimer_real.timer.it_interval);
		TR("_thread_sys_setitimer_out", out->it_value.tv_sec,
		   out->it_value.tv_usec);
	}

	if ((in != NULL) && (in->it_value.tv_sec || in->it_value.tv_usec)) {
		_itimer_real.threadp = selfp;
		_itimer_real.signo = SIGALRM;
		TIMEVAL_TO_TIMESPEC(&in->it_interval,
		   &_itimer_real.timer.it_interval);
		TIMEVAL_TO_TIMESPEC(&in->it_value,
		   &_itimer_real.timer.it_value);
		TR("_thread_sys_setitimer_in", in->it_value.tv_sec,
		   in->it_value.tv_usec);
		_thread_timer_timeout(alarm_handler,
		   (void *)&_itimer_real, &_itimer_real.timer.it_value);
	}
	else {
		_itimer_real.timer.it_value.tv_sec = 0;
		_itimer_real.timer.it_value.tv_nsec = 0;
	}
	_thread_kern_exit();

	TR("_thread_sys_setitimer_end", 0, 0);

	return (0);
}

/*
 * Get the current real-time interval timer.
 * This function can be called with the scheduler locked.
 */
void
_thread_getitimer(out)
	struct itimerval *out;
{
	struct timespec ts;

	/* Copy in the reload value */
	TIMESPEC_TO_TIMEVAL(&out->it_interval, &_itimer_real.timer.it_interval);

	if ((_itimer_real.timer.it_value.tv_sec == 0) && 
	    (_itimer_real.timer.it_value.tv_nsec == 0))
		timerclear(&out->it_value);
	else {
		/* Fetch the time left on the current timer and convert it. */
		_thread_timer_timeleft(alarm_handler, &_itimer_real, &ts);
		TIMESPEC_TO_TIMEVAL(&out->it_value, &ts);
	}
}

int
_thread_sys_getitimer(how, out)
	int how;
	struct itimerval *out;
{

	TR("_thread_sys_getitimer", how, out);

	if ((how == ITIMER_VIRTUAL) || (how == ITIMER_PROF))
		return (_syscall_sys_getitimer(how, out));

	if (how != ITIMER_REAL)
		return (EINVAL);

	_thread_kern_enter();
	_thread_getitimer(out);
	_thread_kern_exit();
	return (0);
}

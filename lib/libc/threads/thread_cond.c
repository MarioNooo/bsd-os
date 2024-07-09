/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_cond.c,v 1.17 2003/06/17 18:06:07 giff Exp
 */

/*
 * POSIX 1c pthread condition variable implementation.
 *
 * There are some tricky bits in here.  Condition variables must be
 * associated with a guard mutex.  Some of the operations need to
 * be done atomically.  Mutexes can also be used alone so the standard
 * routines create critical sections by locking the monitor.  We can't
 * do that here because we'd miss a possible condition signal between
 * the mutex lock and the condition block.  The side effect is that this
 * code knows more about mutexes than it should.  I could have made 
 * wrappers for the mutex functions and done non-premptive versions
 * but that seemed like too much overkill...
 *
 * TODO:	This implementation doesn't currently deal with the 
 *		PSHARED attribute although its desireable.  
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_cond_init(cond, cond_attr)
	pthread_cond_t * cond;
	const pthread_condattr_t * cond_attr;
{

	TR("pthread_cond_init", cond, cond_attr);

	if (!THREAD_SAFE())
		THREAD_INIT();

	_thread_kern_enter();

	if (cond == NULL) {
		_thread_kern_exit();
		return (EINVAL);
	}

	bzero(cond, sizeof(pthread_cond_t));
	_thread_queue_init(&cond->c_queue);
	cond->c_flags = COND_FLAGS_INITED;
	if (cond_attr != NULL)
		cond->c_flags |= cond_attr->c_flags;
	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD,
	    EVENT_THREAD_COND_INIT, cond);
	_thread_kern_exit();
	return (0);
}

int
pthread_cond_destroy(cond)
	pthread_cond_t * cond;
{

	TR("pthread_cond_destroy", cond, 0);

	if (cond == NULL)
		return (EINVAL);

	if (!THREAD_SAFE())
		THREAD_INIT();

	_thread_kern_enter();

	/* Make sure no pthread is waiting on the queue. */
	if (_thread_queue_get(&cond->c_queue) != NULL) {
		_thread_kern_exit();
		return (EBUSY);
	}

	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_COND_DESTROY,
	    cond);
	cond->c_flags = 0;
	_thread_kern_exit();
	return (0);
}

int
pthread_cond_wait(cond, mutex)
	pthread_cond_t * cond;
	pthread_mutex_t * mutex;
{
	struct pthread * selfp;
	int status;

	TR("pthread_cond_wait", cond, mutex);

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	_thread_kern_enter();
	if ((cond == NULL) || (mutex == NULL) ||
	    ((cond->c_flags & COND_FLAGS_INITED) == 0) ||
	    (mutex->m_owner != selfp)) {
		_thread_kern_exit();
		return (EINVAL);
	}

	/* XXX Do this inline so its atomic with the condwait */
	THREAD_EVT_OBJ_USER_2(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_COND_WAIT,
	    cond, mutex);
	mutex->m_owner = _thread_kern_wakeup(&mutex->m_queue);
	_thread_kern_block(&cond->c_queue, PS_COND_WAIT, NULL);

	TR("pthread_cond_wait_blkret1", cond, mutex);

	/* Re-Lock the mutex: */
	status = _thread_mutex_lock(mutex);
	_thread_kern_exit();
	return(status);
}

int
pthread_cond_timedwait(cond, mutex, abstime)
	pthread_cond_t * cond;
	pthread_mutex_t * mutex;
	const struct timespec * abstime;
{
	struct pthread * selfp;
	struct timespec reltime, now;
	int ret, erc;

	TR("pthread_cond_timedwait", cond, mutex);

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	_thread_kern_enter();
	if ((cond == NULL) || (mutex == NULL) ||
	   ((cond->c_flags & COND_FLAGS_INITED) == 0) ||
	   (mutex->m_owner != selfp)) {
		_thread_kern_exit();
		return (EINVAL);
	}

	/*
	 * Lock the condition queue and queue the pthread on it.  
	 * Calculate a relative timeout value from the absolute 
	 * time they passed us.
	 */

	reltime = *abstime;
	clock_gettime(CLOCK_REALTIME, &now);
	timespecsub(&reltime, &now);

	/* XXX Do this inline so its atomic with the condwait */
	THREAD_EVT_OBJ_USER_4(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_COND_TWAIT,
	    cond, mutex, reltime.tv_sec, reltime.tv_nsec );
	mutex->m_owner = _thread_kern_wakeup(&mutex->m_queue);
	erc = _thread_kern_block(&cond->c_queue, PS_COND_WAIT, &reltime);

	TR("pthread_cond_timedwait_blkret1", cond, mutex);
	/*
	 * Wakeup either happened because someone signalled the condition
	 * or because a timeout occured.  Figure out which one and return
	 * the appropriate status.
	 */

	/* Re-Lock the mutex: */
	if ((ret = _thread_mutex_lock(mutex)) != 0)
		return (ret);

	if (erc == ETIMEDOUT) {
		THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD,
		    EVENT_THREAD_COND_TWAIT_TIMEOUT, cond);
		return (ETIMEDOUT);
	}

	return (0);
}

int
pthread_cond_signal(cond)
	pthread_cond_t * cond;
{

	TR("pthread_cond_signal", cond, cond->c_queue);

	if (!THREAD_SAFE())
		THREAD_INIT();

	_thread_kern_enter();
	if (cond == NULL || ((cond->c_flags & COND_FLAGS_INITED) == 0)) {
		_thread_kern_exit();
		return (EINVAL);
	}

	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_COND_SIGNAL,
	    cond);
	_thread_kern_wakeup(&cond->c_queue);
	_thread_kern_exit();

	return (0);
}

int
pthread_cond_broadcast(cond)
	pthread_cond_t * cond;
{
	TR("pthread_cond_broadcast", cond, cond->c_queue);

	if (!THREAD_SAFE())
		THREAD_INIT();

	_thread_kern_enter();
	if (cond == NULL || ((cond->c_flags & COND_FLAGS_INITED) == 0)) {
		_thread_kern_exit();
		return (EINVAL);
	}

	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD,
	    EVENT_THREAD_COND_BROADCAST, cond);
	while (_thread_kern_wakeup(&cond->c_queue) != NULL) 
		continue;

      _thread_kern_exit();
      return (0);
}

int
pthread_condattr_init(cond_attr)
      pthread_condattr_t * cond_attr;
{
      TR("pthread_condattr_init", cond_attr, 0);

      _thread_kern_enter();

      if (cond_attr == NULL) {
              _thread_kern_exit();
              return (EINVAL);
      }
      bzero(cond_attr, sizeof(pthread_condattr_t));
      cond_attr->c_flags |= COND_FLAGS_INITED;
      _thread_kern_exit();
      return (0);
}

int
pthread_condattr_destroy(cond_attr)
      pthread_condattr_t * cond_attr;
{
      TR("pthread_condattr_destroy", cond_attr, 0);

      _thread_kern_enter();

      if (cond_attr == NULL) {
              _thread_kern_exit();
              return (EINVAL);
      }
      cond_attr->c_flags = 0;
      _thread_kern_exit();
      return (0);
}

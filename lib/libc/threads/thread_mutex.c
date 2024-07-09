/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 * 
 * BSDI thread_mutex.c,v 1.12 2003/06/17 18:06:07 giff Exp
 */

/*
 * POSIX 1.c pthread mutex implementation.
 *
 * This implementation doesn't currently deal with the PSHARED
 * attribute although its desireable.  
 *
 * This code has been designed to work correctly even in non-threaded
 * programs. A null selfp is returned if threads has never been 
 * initialized and thread monitor enter/exit become no-ops until
 * threads have been set up.  This might also be useful with a
 * PSHARED mutex since you could use them without using threads.
 * The primary reason though is to allow us to use mutexes with
 * libc code to make other routines re-entrant.
 *
 * TODO:
 *	Could do PTHREAD_PRIORITY_PROTECT mutex attributes...
 */

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_mutex_init(mutex, mutex_attr)
	pthread_mutex_t * mutex;
	const pthread_mutexattr_t * mutex_attr;
{

	TR("pthread_mutex_init", mutex, mutex_attr);

	if (!THREAD_SAFE())
		THREAD_INIT();

	_thread_kern_enter();
	if (mutex == NULL) {
		_thread_kern_exit();
		return (EINVAL);
	}
	bzero(mutex, sizeof(pthread_mutex_t));
	_thread_queue_init(&mutex->m_queue);
	mutex->m_owner = NULL;
	mutex->m_flags = MUTEX_FLAGS_INITED;
	if (mutex_attr != NULL)
		mutex->m_flags |= mutex_attr->m_flags;
	_thread_kern_exit();
	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_MTX_INIT,
	    mutex);
	return (0);
}

int
pthread_mutex_destroy(mutex)
	pthread_mutex_t * mutex;
{
	int ret = 0;

	TR("pthread_mutex_destroy", mutex, 0);

	if (!THREAD_SAFE())
		THREAD_INIT();

	_thread_kern_enter();

	if (mutex == NULL) {
		_thread_kern_exit();
		return (EINVAL);
	}
	/* 
	 * Make sure no pthread owns this mutex and no-one is 
	 * waiting on the queue. Testing m_owner != 0 is enough
	 * since if there is no owner the queue is also empty by
	 * definition.
	 */
	if (mutex->m_owner != NULL) {
		_thread_kern_exit();
		return (EBUSY);
	}

	mutex->m_flags = 0;
	mutex->m_owner = NULL;

	_thread_kern_exit();
	
	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_MTX_DESTROY,
	    mutex);
	return (0);
}

int
pthread_mutex_trylock(mutex)
	pthread_mutex_t * mutex;
{
	int ret;
	struct pthread * selfp;

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	TR("pthread_mutex_trylock", mutex, selfp);

	_thread_kern_enter();

	if (mutex == NULL) {
		_thread_kern_exit();
		return (EINVAL);
	}
	if ((mutex->m_flags & MUTEX_FLAGS_INITED) == 0) {
		_thread_kern_exit();
		return (EINVAL);
	}

	/* Check the state of the mutex. */
	if (mutex->m_owner != NULL)
		ret = EBUSY;	/* Already locked */
	else {
		ret = 0; 	/* Success */
		mutex->m_owner = selfp;
	}

	THREAD_EVT_OBJ_USER_2(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_MTX_TRYLOCK,
	    mutex, ret);

	_thread_kern_exit();
	return (ret);
}

/*
 * Acquire a mutex with kernel already locked.
 */
int
_thread_mutex_lock(mutex)
	pthread_mutex_t * mutex;
{
	struct pthread * selfp;

	if (!THREAD_SAFE())
		abort();		/* Shouldn't be possible */

	selfp = _thread_selfp();

	TR("_thread_mutex_lock", mutex, selfp);

	if (mutex == NULL)
		return (EINVAL);

	if ((mutex->m_flags & MUTEX_FLAGS_INITED) == 0)
		return (EINVAL);

	if (mutex->m_owner == selfp)
		return (EDEADLK);

	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_MTX_LOCK,
	    mutex);
	while (mutex->m_owner != selfp) {
		/* Check if this mutex is not locked: */
		if (mutex->m_owner == NULL) {
			mutex->m_owner = selfp;
			break;
		}

		_thread_kern_block(&mutex->m_queue, PS_MUTEX_WAIT, NULL);
		TR("_thread_mutex_lock_blkret1", mutex, selfp);
	}

	return(0);
}

/*
 * Acquire a mutex.  Kernel is not locked on entry.
 */
int
pthread_mutex_lock(mutex)
	pthread_mutex_t * mutex;
{
	struct pthread * selfp;
	int status;

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	TR("pthread_mutex_lock", mutex, selfp);

	_thread_kern_enter();
	status = _thread_mutex_lock(mutex);
	_thread_kern_exit();

	return (status);
}

int
pthread_mutex_unlock(mutex)
	pthread_mutex_t * mutex;
{
	struct pthread * selfp;

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();

	TR("pthread_mutex_unlock", mutex, selfp);

	_thread_kern_enter();

	if (mutex == NULL) {
		_thread_kern_exit();
		return (EINVAL);
	}
	if ((mutex->m_flags & MUTEX_FLAGS_INITED) == 0) {
		_thread_kern_exit();
		return (EINVAL);
	} 

	if ((mutex->m_owner != selfp) || (mutex->m_owner == NULL)) {
		_thread_kern_exit();
		return (EPERM);
	}

	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_MTX_UNLOCK,
	    mutex);
	mutex->m_owner = _thread_kern_wakeup(&mutex->m_queue);

	TR("pthread_mutex_unlock1", mutex, mutex->m_owner);

	_thread_kern_exit();
	return (0);
}

int
pthread_mutexattr_init(mutex_attr)
      pthread_mutexattr_t * mutex_attr;
{
      TR("pthread_mutexattr_init", mutex_attr, 0);

      _thread_kern_enter();

      if (mutex_attr == NULL) {
              _thread_kern_exit();
              return (EINVAL);
      }
      bzero(mutex_attr, sizeof(pthread_mutexattr_t));
      mutex_attr->m_flags |= MUTEX_FLAGS_INITED;
      _thread_kern_exit();
      return (0);
}

int
pthread_mutexattr_destroy(mutex_attr)
      pthread_mutexattr_t * mutex_attr;
{
      TR("pthread_mutexattr_destroy", mutex_attr, 0);

      _thread_kern_enter();

      if (mutex_attr == NULL) {
              _thread_kern_exit();
              return (EINVAL);
      }
      mutex_attr->m_flags = 0;
      _thread_kern_exit()
      return (0);
}

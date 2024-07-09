/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996, 2001 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI thread_qlock.c,v 1.6 2003/10/16 16:04:51 giff Exp
 */

#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"

/*
 * This code used to be in libc/stdio/flockfile.c, but that is now just
 * stubs that call here.  This code is also used from readdir_r().
 *
 * This is pretty much the same as a pthread mutex, except that "mcount"
 * lets us count recursive locks.
 */
struct qlock {
	struct pthread_queue queue;	/* queue of threads waiting */
	struct pthread *owner;		/* owner of this lock */
	int mcount;			/* recursion count for owner */
};

int
_thread_qlock_init(void **vpp)
{
	void *vp, *memp;
	struct qlock *qp;

	_thread_kern_enter();
	qp = *vpp;
	if (qp == NULL) {
		/* there is no lock structure yet -- get a new one */
		if ((qp = malloc(sizeof *qp)) == NULL) {
			_thread_kern_exit();
			return (-1);	/* failed */
		}
		*vpp = qp;
	}
	_thread_queue_init(&qp->queue);
	qp->owner = NULL;
	qp->mcount = 0;
	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_QLOCK_INIT,
	    qp);
	_thread_kern_exit();
	return (0);			/* succeeded; not locked */
}

void
_thread_qlock_free(void **vpp)
{
	struct qlock *qp;

	_thread_kern_enter();
	if ((qp = *vpp) != NULL) {
		*vpp = NULL;
		/* Wakeup any waiters */
		THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD,
		    EVENT_THREAD_QLOCK_FREE, qp);
		while (_thread_kern_wakeup(&qp->queue))
			continue;
		free(qp);
	}
	_thread_kern_exit();
}

int
_thread_qlock_lock(void **vpp)
{
	struct pthread *selfp;
	struct qlock *qp;

	_thread_kern_enter();
	for (;;) {
		if ((qp = *vpp) == NULL) {
			_thread_kern_exit();
			return (-1);	/* there is no lock */
		}
		selfp = _thread_selfp();
		THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD,
		    EVENT_THREAD_QLOCK_LOCK, qp);
		if (qp->owner == NULL) {
			qp->owner = selfp;
			qp->mcount = 1;
			break;
		}
		if (qp->owner == selfp) {
			qp->mcount++;
			break;
		}
		if (_thread_kern_block(&qp->queue, PS_IO_WAIT, NULL) ==
		    ETHREAD_CANCELED) {
			_thread_cancel();
			/*NOTREACHED*/
		}
	}
	_thread_kern_exit();
	return (0);
}

int
_thread_qlock_try(void **vpp)
{
	struct pthread *selfp;
	struct qlock *qp;
	int ret;

	_thread_kern_enter();
	if ((qp = *vpp) == NULL) {
		_thread_kern_exit();
		return (-1);	/* there is no lock */
	}
	selfp = _thread_selfp();
	if (qp->owner == NULL) {
		qp->owner = selfp;
		qp->mcount = 1;
		ret = 0;	/* lock acquired */
	} else if (qp->owner == selfp) {
		qp->mcount++;
		ret = 0;	/* lock (still) held */
	} else
		ret = -1;	/* lock attempt failed */

	THREAD_EVT_OBJ_USER_2(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_QLOCK_TRY,
	    qp, ret);
	_thread_kern_exit();
	return (ret);
}

int
_thread_qlock_unlock(void **vpp)
{
	struct qlock *qp;

	_thread_kern_enter();
	if ((qp = *vpp) == NULL) {
		_thread_kern_exit();
		return (-1);
	}
	THREAD_EVT_OBJ_USER_1(EVT_LOG_PTRACE_THREAD, EVENT_THREAD_QLOCK_UNLOCK,
	    qp);
	if (qp->owner == _thread_selfp()) {
		if (--qp->mcount <= 0) {
			qp->mcount = 0;
			qp->owner = NULL;
			_thread_kern_wakeup(&qp->queue);
		}
	}
	_thread_kern_exit();
	return (0);
}

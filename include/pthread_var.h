/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 2001 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI pthread_var.h,v 2.3 2003/04/07 13:30:53 giff Exp
 */

/*
 * This file exports just enough of the opaque pthread data type
 * for programs like gdb to be able to chase thread data structures
 * and recover register information about sleeping threads.
 */

#ifndef _PTHREAD_VAR_H_
#define	_PTHREAD_VAR_H_

#include <machine/pthread_var.h>

/* Pthread cancelation status */
enum pthread_canstate {
	PCS_NORMAL = 0,
	PCS_BLOCKED,		/* Cancelation is pending, but disabled */
	PCS_DEFERRED,		/* Deferred cancelation is pending */
	PCS_ASYNCHRONOUS,	/* Async cancelation is pending */
	PCS_CANCELING		/* Thread is being canceled */
};

/* Pthread execution states. */
enum pthread_state {
	PS_RUNNING,
	PS_RUNNABLE,
	PS_SUSPENDED,
	PS_MUTEX_WAIT,
	PS_COND_WAIT,
	PS_IO_WAIT,
	PS_CLOSE_WAIT,
	PS_SELECT_WAIT,
	PS_SLEEP_WAIT,
	PS_WAIT_WAIT,
	PS_SIG_WAIT,
	PS_JOIN_WAIT,
	PS_DEAD
};

struct pthread {
	struct pthread 		*nxt;		/* next in all/dead list */
	struct pthread_queue	*queue;		/* Current queue we are on */
	struct pthread		*qnxt;		/* Our position in the queue */

	void *			(*start_routine) __P((void *));
	void *			arg;
	pthread_attr_t		attr;		/* thread creation attributes */
	int			stacksize;	/* Can be different from attr */
	void *			stackaddr;	/* stack base address */
	thread_machdep_state	saved_context;
	enum pthread_state	state;		/* Thread scheduling state */

	int 			priority;	/* Thread run priority */
	int			schedpolicy;	/* Scheduling policy */
	int			error;		/* Thread local errno */

	int			flags;
#define PTHREAD_DETACHED        0x0001 		/* Thread is detached */
#define PTHREAD_JOINED          0x0002		/* Thread has been Joined */
#define PTHREAD_ZOMBIE          0x0004		/* Dead, status not fetched */
#define	PTHREAD_TIMEDOUT	0x0010		/* A timed operation timedout */
#define	PTHREAD_INTERRUPTED	0x0020		/* An I/O operation was intr. */
#define	PTHREAD_SUSPEND_PENDING	0x0040		/* A suspend is pending */

	/* Per thread signal handling state */
	sigset_t		sigmask;	/* per-thread sigmask */
	sigset_t		sigpend;	/* per-thread pending signals */

	/* Miscellaneous data. */
	struct timespec		timer;		/* current active timer */
	struct pthread *	jthread;	/* they called join on us */
	void *			status;		/* For pthread_join */
	void **			specific_data;	/* thr specific data ptrs */
	struct pthread_cleanup *cleanup;	/* cleanup handlers */
	fd_set			*ignore;	/* For select, when fd closes */

	/* cancelability status */
	int			can_flags;	/* cancelability state/type */
	enum pthread_canstate	can_state;	/* cancelation is pending */
};

#endif	/* _PTHREAD_VAR_H_ */

/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996, 2001 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_private.h,v 1.25 2003/07/08 21:52:13 polk Exp
 */

#ifndef _THREAD_PRIVATE_H
#define _THREAD_PRIVATE_H

#include <sys/types.h>

#include <signal.h>
#include <time.h>

#include "thread_aio.h"
#include "thread_list.h"
#include "thread_machdep.h"
#include "thread_signal.h"
#include "thread_syscall.h"
#include "thread_trace.h"
#include "thread_evtlog.h"

/*
 * Library Initialization.
 * 
 * THREAD_INIT() arranges to call _thread_init if the library has 
 * never been initialised.  This macro is used by other pthreads
 * routines to make sure the library is set up before other
 * pthread calls are made.
 *
 * THREAD_SAFE returns TRUE if the library has been initialized.
 * Its used by other libc code to detect the need to perform 
 * threadsafe locking.
 */

extern pthread_once_t _thread_init_once;
extern int _threads_initialized;
#ifdef DEBUG
extern int (*_thread_death)();
#endif

/* For _thread_kern_switch() */
enum switch_signal {SWITCH_SIGNALS, SWITCH_NOSIGNALS};

#define	THREAD_INIT() \
	(void) pthread_once(&_thread_init_once, _thread_init)

#define	THREAD_SAFE() \
	(_threads_initialized != 0)

/* 
 * Macros for often used functions 
 */

#define	_thread_selfp()		(_thread_run)

#define	_thread_seterrno(thr,erc) 			\
	(((thr) == _thread_selfp() ? errno : (thr)->error) = (erc))

#if WV_INST
#define	_thread_kern_sched_state(threadp,x)		 		\
	do {								\
	THREAD_EVT_OBJ_USER_2_IF(((threadp)->state != (x)),		\
	    EVT_LOG_PTRACE_THREAD, EVENT_THREAD_STATE, threadp, x);	\
	((threadp)->state = (x));					\
	} while (0)
#else
#define _thread_kern_sched_state(threadp,x)				\
	((threadp)->state = (x))
#endif /* WV_INST */

/*
 * The call to abort is to catch problems with
 * re-entering the thread monitor when its 
 * already locked.
 */
#ifdef DEBUG
#define	_thread_kern_enter() {				\
	if (_thread_kern_owner != NULL)			\
		abort();				\
	_thread_kern_owner = _thread_selfp();		\
	if (_thread_death && (_thread_death)()) {			\
		TR("_thread_death", _thread_death, (_thread_death)()); \
		abort();				\
		} \
	}
#else
#define	_thread_kern_enter() {				\
	if (_thread_kern_owner != NULL)			\
		abort();				\
	_thread_kern_owner = _thread_selfp();		\
	}
#endif

#ifdef DEBUG
#define	_thread_kern_exit() {				\
        if (_thread_kern_owner == NULL && _thread_selfp()) \
		abort();				\
	if (_thread_death && (_thread_death)()) {			\
		TR("_thread_death", _thread_death, (_thread_death)()); \
		abort();				\
		} \
	if (SIGNOTEMPTY(_thread_sig_blocked) ||		\
	    (_thread_kern_need_resched != 0))		\
		_thread_kern_switch(SWITCH_SIGNALS);	\
	_thread_kern_owner = NULL;			\
	}
#else
#define	_thread_kern_exit() {				\
	if (SIGNOTEMPTY(_thread_sig_blocked) ||		\
	    (_thread_kern_need_resched != 0))		\
		_thread_kern_switch(SWITCH_SIGNALS);	\
	_thread_kern_owner = NULL;			\
	}
#endif

#define	_thread_kern_exit_always() {			\
		_thread_kern_owner = NULL;		\
	}

#ifdef DEBUG
#define _thread_kern_check() {                          \
        if (_thread_kern_owner == NULL)                 \
                abort();                                \
	}
#else
#define _thread_kern_check()
#endif

#define	_thread_kern_locked() \
	(_thread_kern_owner != NULL)

#define	_THREAD_TESTCANCEL() 				\
	(_thread_selfp() != NULL &&			\
	    _thread_selfp()->can_state == PCS_DEFERRED)

#define	_thread_testcancel() {				\
	if (_THREAD_TESTCANCEL()) {			\
		_thread_cancel();			\
	}						\
	}

/* 
 * Error code returned by _thread_kern_block to indicate a thread has
 * been canceled.
 */
#define	ETHREAD_CANCELED	-42

#define	THREAD_SCHED_SECS	0
#define	THREAD_SCHED_USECS	200000		/* 200 msec */

struct pthread_cleanup {
	struct 	pthread_cleanup * next;
	void	(*routine) __P((void *));
	void	*arg;
};

struct pthread_atfork {
	struct 	pthread_atfork * next;
	void	(*routine) __P((void));
};

/* Exported pthread information, including 'struct pthread'.  */
#include <pthread_var.h>

/*
 * Miscellaneous definitions.
 */
#define PTHREAD_DATAKEYS_MAX			256
#define	PTHREAD_DESTRUCTOR_ITERATIONS		16
#define PTHREAD_MAX_PRIORITY			255
#define PTHREAD_MIN_PRIORITY			0
#define PTHREAD_DEFAULT_PRIORITY		64

/*
 * Timer macros
 */
#define	timevalinvalid(tvp) \
	((tvp)->tv_sec < 0 || (tvp)->tv_sec > 100000000 || \
	  (tvp)->tv_usec < 0 || (tvp)->tv_usec >= 1000000)
#define	timespecinvalid(tsp) \
	((tsp)->tv_sec < 0 || (tsp)->tv_sec > 100000000 || \
	  (tsp)->tv_nsec < 0 || (tsp)->tv_nsec >= 1000000000)

/*
 * Global variables for the thread kernel.
 */

#if (!WV_INST)
extern struct pthread *_thread_run;
#endif /* WV_INST */
extern struct pthread *_thread_kern_owner;
extern int _thread_kern_need_resched;
extern struct pthread_queue _thread_runq;
extern struct pthread_queue _thread_waitq;
extern struct pthread_atfork *_thread_fork_prepare;
extern struct pthread_atfork *_thread_fork_parent;
extern struct pthread_atfork *_thread_fork_child;

/*
 * Function prototype definitions.
 */
__BEGIN_DECLS
int	_thread_atfork __P(( void (*) __P((void)), void (*) __P((void)), 
		void (*) __P((void))));
void	_thread_cancel __P((void));
int 	_thread_create __P((struct pthread *, pthread_t *, 
		const pthread_attr_t *, void *(*) __P((void *)), void *));
int 	_thread_findp __P((struct pthread **, pthread_t));
void	_thread_getitimer __P((struct itimerval *));
void 	_thread_init __P((void));
int 	_thread_kern_block __P((struct pthread_queue *, enum pthread_state, 
		struct timespec *));
void	_thread_kern_cleanup_child __P((void));
void	_thread_kern_cleanup_dead __P((void));
void	_thread_kern_enter __P((void));
int 	_thread_kern_init __P((void));
int	_thread_kern_locked __P((void));
void 	_thread_kern_setrunnable __P((struct pthread *));
void	_thread_kern_start __P((void));
void 	_thread_kern_switch __P((enum switch_signal));
struct 	pthread *_thread_kern_wakeup __P((struct pthread_queue *));
void 	_thread_key_cleanupspecific __P((void));
int	_thread_mutex_lock __P((pthread_mutex_t *));
struct 	pthread *_thread_queue_deq __P((struct pthread_queue *));
void 	_thread_queue_enq __P((struct pthread_queue *, struct pthread *));
struct 	pthread *_thread_queue_get __P((struct pthread_queue *));
int 	_thread_queue_init __P((struct pthread_queue *));
int 	_thread_queue_remove __P((struct pthread_queue *, struct pthread *));
int 	_thread_queue_member __P((struct pthread_queue *, struct pthread *));
struct 	pthread *_thread_selfp __P((void));
void 	_thread_seterrno __P((struct pthread *, int));
int 	_thread_timer_dotimeouts __P((int));
int 	_thread_timer_timeleft __P((void (*) __P((void *)), void *, 
		struct timespec *));
int 	_thread_timer_timeout __P((void (*) __P((void *)), void *, 
		struct timespec *));
int 	_thread_timer_untimeout __P((void (*) __P((void *)), void *, 
		struct timespec *));
int	_thread_sigmask __P((int, const sigset_t *, sigset_t *));
int 	_thread_once __P((pthread_once_t *, void (*) __P((void))));

/*
 * This is a simple queueing lock interface that thread-enabled
 * code elsewhere in the C library can use.  To use it, the caller
 * simply needs to provide a "void *" object, initially NULL, and
 * then call us to manage the locking and queueing (passing the
 * address of the "void *", which we fill in with private data).
 *
 * ??? maybe we should do read/writer locks? this suffices for now
 *
 * Except for free (no return value) and unlock, each function
 * returns 1 if now locked, -1 if there is no lock, and 0 if there
 * is a lock but it is not now locked (i.e., is unlocked or, for
 * try(), is busy).  The unlock function returns 0 (unlocked but
 * maybe re-locked recursively or by someone else since), or -1 (no lock).
 *
 * This is almost the same as the pthread_mutex code, except that it
 * handles lock recursion, and is a bit lighter-weight.
 */
int	_thread_qlock_init(void **);
void	_thread_qlock_free(void **);
int	_thread_qlock_lock(void **);
int	_thread_qlock_try(void **);
int	_thread_qlock_unlock(void **);
__END_DECLS

#endif  

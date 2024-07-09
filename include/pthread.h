/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI pthread.h,v 1.9 1999/10/08 12:49:15 jch Exp
 */

#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#include <sys/types.h>

#include <sched.h>
#include <signal.h>
#include <time.h>

struct pthread;

struct pthread_queue {
	struct pthread 	*q_head;
};

#define PTHREAD_QUEUE_INITIALIZER { 	\
	NULL 				\
	}

/* 
 * Mutex definitions.
 */

struct pthread_mutex {
	int				m_flags;
	struct pthread_queue		m_queue;
	struct pthread * 		m_owner;
};

/*
 * Mutex Flags:
 */

#define MUTEX_FLAGS_INITED	0x0001
#define MUTEX_FLAGS_PSHARED	0x0002		/* Mutex is Process shared */

/*
 * Static mutex initialization values. 
 */
#define PTHREAD_MUTEX_INITIALIZER  { 	\
	MUTEX_FLAGS_INITED, 		\
	PTHREAD_QUEUE_INITIALIZER, 	\
	NULL 				\
	}

struct pthread_mutex_attr {
	int			m_flags;
};

/* 
 * Condition variable definitions.
 */

struct pthread_cond {
	struct pthread_queue	c_queue;
	int			c_flags;
};

struct pthread_cond_attr {
	int			c_flags;
};

/*
 * Flags for condition variables.
 */
#define COND_FLAGS_INITED	0x0001
#define COND_FLAGS_PSHARED	0x0002

/*
 * Static cond initialization values. 
 */
#define PTHREAD_COND_INITIALIZER {		\
	PTHREAD_QUEUE_INITIALIZER, 		\
	COND_FLAGS_INITED 			\
	}

/* Pthread Creation Attributes */
#define	_POSIX_THREAD_ATTR_STACKSIZE	1
#define	_POSIX_THREAD_ATTR_STACKADDR	1

#define PTHREAD_ATTR_INITED             0x0001
#define PTHREAD_CREATE_DETACHED         0x0002
#define PTHREAD_CREATE_JOINABLE         0x0004
#define	PTHREAD_CREATE_SUSPENDED	0x0008		/* NOT PORTABLE */
#define	PTHREAD_INHERIT_SCHED		0x0010
#define	PTHREAD_EXPLICIT_SCHED		0x0020
#define	PTHREAD_SCOPE_SYSTEM		0x0040
#define	PTHREAD_SCOPE_PROCESS		0x0080

#define	PTHREAD_STACK_MIN		1024
#define	PTHREAD_STACK_DEFAULT		65536

struct pthread_attr {
	int			flags;
	void *			stackaddr;
	size_t			stacksize;
	int			schedpolicy;
	struct	sched_param	schedparam;
};

/*
 * Pthread Once definitions.
 */
struct pthread_once {
	int			state;
};

/*
 * Flags for once initialization.
 */
#define PTHREAD_NEEDS_INIT  0
#define PTHREAD_DONE_INIT   1
#define PTHREAD_INIT_INIT   2

/*
 * Static once initialization values. 
 */
#define PTHREAD_ONCE_INIT { 			\
	PTHREAD_NEEDS_INIT 			\
	}

/*
 * Pthread scheduling priority values
 */
#define PTHREAD_SCHED_MAX_PRIORITY		255
#define PTHREAD_SCHED_MIN_PRIORITY		0
#define PTHREAD_SCHED_DEFAULT_PRIORITY		64

/*
 * Pthread cancelability enable values
 */
#define	PTHREAD_CANCEL_ENABLE			0
#define	PTHREAD_CANCEL_DISABLE			1

/*
 * Pthread cancelability type values
 */
#define	PTHREAD_CANCEL_DEFERRED			0
#define	PTHREAD_CANCEL_ASYNCHRONOUS		2

/*
 * Value returned by pthread_join() when the target thread was canceled
 */
#define	PTHREAD_CANCELED			((void *) -42)

/*
 * Type definitions.
 */
typedef int     			pthread_key_t;
typedef struct	pthread *		pthread_t;
typedef struct	pthread_attr		pthread_attr_t;
typedef struct	pthread_cond		pthread_cond_t;
typedef struct	pthread_cond_attr	pthread_condattr_t;
typedef struct	pthread_mutex		pthread_mutex_t;
typedef struct	pthread_mutex_attr	pthread_mutexattr_t;
typedef struct	pthread_once		pthread_once_t;

/*
 * Thread function prototype definitions:
 */
__BEGIN_DECLS
int	pthread_atfork __P(( void (*) __P((void)), void (*) __P((void)), 
		void (*) __P((void))));
int	pthread_attr_init __P((pthread_attr_t *));
int	pthread_attr_destroy __P((pthread_attr_t *));
int	pthread_attr_getdetachstate __P((pthread_attr_t *, int *));
int	pthread_attr_getinheritsched __P((pthread_attr_t *, int *));
int	pthread_attr_getschedparam __P((pthread_attr_t *, 
		struct sched_param *));
int	pthread_attr_getschedpolicy __P((pthread_attr_t *, int *));
int	pthread_attr_getscope __P((pthread_attr_t *, int *));
int	pthread_attr_getstackaddr __P((pthread_attr_t *, void **));
int	pthread_attr_getstacksize __P((pthread_attr_t *, size_t *));
int	pthread_attr_getsuspendstate_np  __P((pthread_attr_t *, int *));
int	pthread_attr_setdetachstate __P((pthread_attr_t *, int));
int	pthread_attr_setinheritsched __P((pthread_attr_t *, int));
int	pthread_attr_setschedparam __P((pthread_attr_t *, 
		const struct sched_param *));
int	pthread_condattr_destroy __P((pthread_condattr_t *));
int	pthread_condattr_init __P((pthread_condattr_t *));
int	pthread_attr_setschedpolicy __P((pthread_attr_t *, int));
int	pthread_attr_setscope __P((pthread_attr_t *, int));
int	pthread_attr_setstackaddr __P((pthread_attr_t *, void *));
int	pthread_attr_setstacksize __P((pthread_attr_t *, size_t));
int	pthread_attr_setsuspendstate_np  __P((pthread_attr_t *, int));
int	pthread_cancel __P((pthread_t));
int	pthread_cleanup_push __P((void (*) __P((void *)), void *));
void 	pthread_cleanup_pop __P((int));
int	pthread_cond_broadcast __P((pthread_cond_t *));
int	pthread_cond_destroy __P((pthread_cond_t *));
int	pthread_cond_init __P((pthread_cond_t *, const pthread_condattr_t *));
int	pthread_cond_signal __P((pthread_cond_t *));
int	pthread_cond_timedwait __P((pthread_cond_t *, pthread_mutex_t *, 
		const struct timespec *));
int	pthread_cond_wait __P((pthread_cond_t *, pthread_mutex_t *));
int	pthread_create __P((pthread_t *, const pthread_attr_t *, 
		void *(*) __P((void *)), void *));
int	pthread_detach __P((pthread_t));
int	pthread_equal __P((pthread_t, pthread_t));
void	pthread_exit __P((void *));
int	pthread_getschedparam __P((pthread_t, int *, struct sched_param *));
void *	pthread_getspecific __P((pthread_key_t));
int	pthread_getstackbase_np __P((pthread_t, int *));
int	pthread_mutexattr_destroy __P((pthread_mutexattr_t *));
int	pthread_mutexattr_init __P((pthread_mutexattr_t *));
int	pthread_getstackpointer_np __P((pthread_t, int *));
int	pthread_getstacksize_np __P((pthread_t, int *));
int	pthread_init __P((void));	/* DEPRECATED */
int	pthread_join __P((pthread_t, void **));
int	pthread_key_create __P((pthread_key_t *, void (*) __P((void *))));
int	pthread_key_delete __P((pthread_key_t));
int	pthread_kill __P((pthread_t, int));
int	pthread_mutex_destroy __P((pthread_mutex_t *));
int	pthread_mutex_init __P((pthread_mutex_t *, 
		const pthread_mutexattr_t *));
int	pthread_mutex_lock __P((pthread_mutex_t *));
int	pthread_mutex_trylock __P((pthread_mutex_t *));
int	pthread_mutex_unlock __P((pthread_mutex_t *));
int	pthread_once __P((pthread_once_t *, void (*) __P((void))));
pthread_t pthread_self __P((void));
int	pthread_resume_np __P((pthread_t));
int	pthread_resume_all_np __P((void));
int	pthread_setcancelstate __P((int, int *));
int	pthread_setcanceltype __P((int, int *));
int	pthread_setschedparam __P((pthread_t, int, const struct sched_param *));
int	pthread_setspecific __P((pthread_key_t, const void *));
int	pthread_sigmask __P((int, const sigset_t *, sigset_t *));
int	pthread_suspend_np __P((pthread_t));
int	pthread_suspend_all_np __P((void));
void	pthread_testcancel __P((void));
int	pthread_yield __P((void));		/* DEPRECATED */
__END_DECLS

#endif

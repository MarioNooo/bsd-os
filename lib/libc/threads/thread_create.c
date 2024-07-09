/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_create.c,v 1.19 2003/08/28 21:22:33 giff Exp
 */

#include <sys/types.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_list.h"
#include "thread_signal.h"
#include "thread_syscall.h"
#include "thread_trace.h"

extern pthread_attr_t pthread_attr_default;
extern void **_thread_initial_specific_data;
extern int *_thread_sigcount;	/* per signal count of threads masks */

int _thread_count;	/* count of active threads */

#define	SCHEDATTR_MASK	(		\
	PTHREAD_SCOPE_SYSTEM | 		\
	PTHREAD_SCOPE_PROCESS |		\
	PTHREAD_INHERIT_SCHED | 	\
	PTHREAD_EXPLICIT_SCHED		\
	)

/*
 * This function exists solely to provide a way for gdb to
 * find out about new threads.
 */
void
_thread_create_debugger_hook()
{
	static void *thread_debugger_info[] = {
		&_thread_run,
		&_thread_allthreads,
		NULL
	};

	ptrace(PT_EVENT, 0, (caddr_t)&thread_debugger_info, 0);
}

void
_thread_create_wdb_hook()
{
static void *thread_wdb_info[] = {
		&_thread_run,
		&_thread_allthreads,
		&_thread_deadthreads
	};

	ptrace(PT_EVENT, 0, (caddr_t)&thread_wdb_info, PT_EVT_WDB_CREATE);
}

/*
 * Create a new thread with attributes specified by "attr" within the
 * current process. If "attr" is null apply the default attributes 
 * during thread creation.  The "selfp" parameter indicates the
 * parent thread from which we inherit scheduling attributes.  If
 * "selfp" is NULL we are creating the initial thread in the system.
 *
 * This "low level" routine assumes the thread kernel is locked.
 * It's only called from thread_init and from pthread_create below.
 */
int
_thread_create(selfp, thread, attr, start_routine, arg)
	struct pthread * selfp;
	pthread_t * thread;
	const pthread_attr_t * attr;
	void * (*start_routine) (void *);
	void * arg;
{
	struct pthread * new_thread;
	int i;

	TR("_thread_create", selfp, thread);

	if ((attr != NULL) && ((attr->flags & PTHREAD_ATTR_INITED) == 0))
		return (EINVAL);

	/* Cleanup any dead threads to free up some resources */
	_thread_kern_cleanup_dead();

	/* Allocate memory for the thread structure: */
	if ((new_thread = malloc(sizeof(struct pthread))) == NULL)
		return (EAGAIN);

	bzero(new_thread, sizeof(struct pthread));

	/* Initialize thread attributes. Use default if none provided */
	if (attr == NULL)
		attr = &pthread_attr_default;

	memcpy(&new_thread->attr, attr, sizeof(pthread_attr_t));

	/* 
	 * See if need to inherit scheduling attributes. We'll copy 
	 * these from the parent thread pointer.  Careful to mask 
	 * off and only copy scheduling flags.  The rest of the
	 * flags should come from the thread creation attribute object.
	 */
	if ((new_thread->attr.flags & PTHREAD_INHERIT_SCHED) != 0) {
		attr = &selfp->attr;
		new_thread->attr.flags &= ~SCHEDATTR_MASK;
		new_thread->attr.flags |= (attr->flags & SCHEDATTR_MASK);
		new_thread->attr.schedpolicy = attr->schedpolicy;
		new_thread->attr.schedparam = attr->schedparam;
	}

	new_thread->priority = new_thread->attr.schedparam.sched_priority;
	new_thread->schedpolicy = new_thread->attr.schedpolicy;
	new_thread->start_routine = start_routine;
	new_thread->arg = arg;

	if ((new_thread->attr.flags & PTHREAD_CREATE_DETACHED) != 0) 
		new_thread->flags |= PTHREAD_DETACHED;
	
	/* No signals pending in new thread */
	SIGEMPTYSET(new_thread->sigpend);

	/*
	 * If selfp == NULL the we are creating the initial
	 * thread and need to handle stack and signal mask
	 * stuff specially 
	 */
	if (selfp == NULL) {
		new_thread->state = PS_RUNNING;
		_thread_list_add(&_thread_allthreads, new_thread);
		_thread_run = new_thread;	/* XXX single processor only */
		THREAD_OBJ_USER_INIT();
		THREAD_EVT_OBJ_USER_4(EVT_LOG_PTRACE_THREAD,
		    EVENT_THREAD_CREATE, new_thread, new_thread->start_routine,
		    new_thread->priority, new_thread->state);

		/* 
		 * The non-threaded specific data gets assigned to the
		 * initial thread.
		 */
		new_thread->specific_data = _thread_initial_specific_data;
		_thread_initial_specific_data = NULL;

		/* Initialize the signal mask from the process mask */
		_syscall_sys_sigprocmask(SIG_SETMASK, NULL, &new_thread->sigmask);

		TR("_thread_create_sigprocmask", 0, 0);
	} else {
		void *	base;
		int  	size;

		THREAD_EVT_OBJ_USER_4(EVT_LOG_PTRACE_THREAD,
		    EVENT_THREAD_CREATE, new_thread, new_thread->start_routine,
		    new_thread->priority, new_thread->state);

		/* Initialize signal mask from parent thread */
		new_thread->sigmask = selfp->sigmask;

		/* 
	 	 * Check if a stack was specified in the thread attributes: 
	 	 * If yes use it otherwise go ahead and allocate a new one.
	 	 */
		size = new_thread->attr.stacksize;
		if ((base = new_thread->attr.stackaddr) == NULL) {
			if (_thread_machdep_stack_alloc(size, &base) != 0) {
				free(new_thread);
				return (EAGAIN);
			}
		}

		new_thread->stacksize = size;
		new_thread->stackaddr = base;

		/* 
	 	 * Initialise the saved execution context and set up 
	 	 * a new stack frame so that it looks like it returned 
		 * from a restctx() to the beginning of _thread_kern_start(). 
		 * Since we are going to muck with the PC and SP values 
		 * we won't ever return to here from a context switch.
		 */
		_thread_machdep_savectx(&new_thread->saved_context);
		_thread_machdep_setpc(new_thread, _thread_kern_start);
		_thread_machdep_initstk(new_thread);

		_thread_list_add(&_thread_allthreads, new_thread);
		if (new_thread->attr.flags & PTHREAD_CREATE_SUSPENDED)
			_thread_kern_sched_state(new_thread, PS_SUSPENDED);
		else
			_thread_kern_setrunnable(new_thread);
	}

	/* update the signal counts */
	_thread_count++;
	for (i = 1; i < NSIG; i++) {
		if (SIGISMEMBER(new_thread->sigmask,i))
			++_thread_sigcount[i];
	}

	/* Return a pointer to the thread structure: */
	if (thread != NULL)
		(*thread) = new_thread;

	_thread_create_debugger_hook();
	_thread_create_wdb_hook ();

	return (0);
}

int
pthread_create(thread, attr, start_routine, arg)
	pthread_t * thread;
	const pthread_attr_t * attr;
	void * (*start_routine) __P((void *));
	void * arg;
{
	int ret;
	struct pthread * selfp;

	TR("pthread_create", thread, attr);

	if (!THREAD_SAFE())
		THREAD_INIT();

	selfp = _thread_selfp();
	_thread_kern_enter();
	ret = _thread_create(selfp, thread, attr, start_routine, arg);
	_thread_kern_exit();
	return (ret);
}

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 * 
 * BSDI thread_stack.c,v 1.6 1997/07/25 20:24:31 pjd Exp
 */

/* 
 * Stack allocation and management routines for the thread library.
 *
 * This code uses mmap to allocate stacks to avoid allocating real memory
 * until the pages are actually used.  We also plant a redzone to detect
 * stack overflow in the application.
 */

#include <sys/param.h>
#include <sys/mman.h>

#include <errno.h>
#include <pthread.h>

#include "thread_private.h"
#include "thread_trace.h"

#define	PROT_ALL	(PROT_READ|PROT_WRITE|PROT_EXEC)
#define	MAP_STACK	(MAP_ANON|MAP_PRIVATE)

int
_thread_machdep_stack_alloc(size, addr)
	int size;
	void **addr;
{
	caddr_t stackaddr;

	TR("_thread_machdep_stack_alloc", size, addr);

	if (addr == NULL)
		return (EFAULT);

	/* Round up to a multiple of machine page size */
	size = (((size / NBPG) + ((size % NBPG) ? 1 : 0)) * NBPG);

	/* Add one more page for redzone */
	size += NBPG;

	stackaddr = mmap(0, size, PROT_ALL, MAP_STACK, -1, 0);
	if (stackaddr == ((caddr_t) -1))
		return (ENOMEM);

	/* Plant a REDZONE to detect stack overflow at the base of the stack */
	mprotect(stackaddr, NBPG, PROT_NONE);

	/* Adjust the base past the redzone */
	*addr = (stackaddr + NBPG);
	return (0);
}

int 
_thread_machdep_stack_free(size, addr)
	int size;
	void *addr;
{
	caddr_t stackaddr;

	TR("_thread_machdep_stack_free", size, addr);

	/* Round up to a multiple of machine page size */
	size = (((size / NBPG) + ((size % NBPG) ? 1 : 0)) * NBPG);

	/* Add one more page for redzone */
	size += NBPG;

	/* Adjust stack base for our redzone */
	stackaddr = (addr - NBPG);

	/* And unmap it */
	if (munmap(stackaddr, size) != 0)
		return (errno);
	else
		return (0);
}

void _thread_machdep_initstk(new_thread)
	struct pthread * new_thread;
{
	void * stack = new_thread->stackaddr;
	int stacksize = new_thread->stacksize;

	TR("_thread_machdep_initstk", new_thread, stack);

	/* Stack grows downward */
	stack = ((stack + stacksize) - sizeof(double));
	_thread_machdep_setsp(new_thread, stack);
	_thread_machdep_setfp(new_thread, stack);
}

/* For Java JDK */
void *
_thread_machdep_getsp(thr)
	struct pthread *thr;
{
	return ((void *) thr->saved_context.regs[_THREAD_I386_ESP]);
}

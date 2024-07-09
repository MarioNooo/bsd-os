/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_machdep.h,v 1.4 2001/10/03 17:29:54 polk Exp
 */

#ifndef _MACHINE_THREADS_H_
#define _MACHINE_THREADS_H_

/* Machine-dependent thread state for (e.g.) gdb.  */
#include <machine/pthread_var.h>

/*
 * The odd 8-byte-offset is because saved PCs normally point at the call
 * instructions.  When someone is calling _thread_machdep_setpc, they
 * really want to set things up to return to the argument, not to the
 * argument plus 8, so we have to adjust it here.
 */
#define	_thread_machdep_setpc(thr,pc) \
	(thr)->saved_context.regs[_THREAD_SPARC_PC] = ((unsigned int)(pc) - 8)

#define	_thread_machdep_setsp(thr,sp) \
	(thr)->saved_context.regs[_THREAD_SPARC_SP] = ((unsigned int)(sp))

#define	_thread_machdep_setfp(thr,fp) \
	(thr)->saved_context.regs[_THREAD_SPARC_FP] = ((unsigned int)(fp))

__BEGIN_DECLS
void *	_thread_machdep_getsp __P((struct pthread *));
void 	_thread_machdep_initstk __P((struct pthread *));
int  	_thread_machdep_savectx __P((thread_machdep_state *));
int	_thread_machdep_stack_alloc __P((int, void **));
int	_thread_machdep_stack_free __P((int, void *));
void 	_thread_machdep_restctx __P((thread_machdep_state *, int));
__END_DECLS

#endif 

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI machdep_sync.s,v 1.2 1997/07/17 05:28:11 donn Exp
 */

	.file "machdep_sync.s"

/*
 * Posix 1j (Draft 5) spinlock implementation for ix86 processors.
 */

#include "DEFS.h"

#define	EBUSY		16		/* From errno.h */

/*
 * int spin_init(spinlock_t *m)
 *
 * Returns:	0 = 	Success
 * 
 * Initialize a spin lock data type.
 * This routine never fails.
 */
ENTRY(spin_init)
	movl	4(%esp),%eax		/* point at mutex */
	movl	$0,(%eax)		/* initialize it to 0 */
	xorl	%eax,%eax		/* return success */
	ret

/*
 * int spin_destroy(spinlock_t *m)
 *
 * Returns:	0  	Success
 *		EBUSY	Another locker currently holds the spinlock
 *
 * Destroy a spin lock data type.  This does pretty much what spin_init
 * does but will additionally check to see that the lock is unused. As
 * a side effect it leave the spinlock locked so that an init (or unlock
 * if you are cheating) is required before re-use.
 */
ENTRY(spin_destroy)
	movl	4(%esp),%ecx		/* point at mutex */
	movl	$1,%eax			/* set locked value in acc */
	xchg	%eax,(%ecx)		/* swap with mutex, xchg locks bus */
	cmpl	$0,%eax			/* test for 1 (locked) */
	je	1f			/* Is it currently locked? */
	movl	$EBUSY,%eax		/* Error return */
1:	ret				/* %eax has return value */

/*
 * int spin_lock(spinlock_t *m)
 *
 * Returns:	0 = 	Success
 * 
 * Spin until a spinlock can be aquired.
 * This routine never fails.
 */
ENTRY(spin_lock)
	movl	4(%esp),%ecx		/* point at mutex */
1:	movl	$1,%eax			/* set locked value in acc */
	xchg	%eax,(%ecx)		/* swap with mutex, xchg locks bus */
	cmpl	$0,%eax			/* Was it unlocked? */
	jne	1b			/* If no spin until it is */
	ret				/* %eax holds 0 for the return value */

/*
 * int spin_trylock(spinlock_t *m)
 *
 * Returns:	0  	Success
 *		EBUSY	Another locker currently holds the spinlock
 *
 * Try to lock a spinlock, returning 0 on success or EBUSY if another
 * caller currently holds the spinlock.
 */
ENTRY(spin_trylock)
	movl	4(%esp),%ecx		/* point at mutex */
	movl	$1,%eax			/* set locked value in acc */
	xchg	%eax,(%ecx)		/* swap with mutex, xchg locks bus */
	cmpl	$0,%eax			/* test for 1 (locked) */
	je	1f			/* Is it currently locked? */
	movl	$EBUSY,%eax		/* Error return */
1:	ret

/*
 * int spin_unlock(spinlock_t *m)
 *
 * Returns:	0 = 	Success
 * 
 * Unlock a currently locked spinlock.
 * This routine never fails.
 */
ENTRY(spin_unlock)
	movl	4(%esp),%ecx		/* point at mutex */
	xorl	%eax,%eax		/* set unlocked value in acc */
	xchg	%eax,(%ecx)		/* swap with mutex, xchg locks bus */
	xorl	%eax,%eax		/* zero the acc for success ret code */
	ret

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI machdep_sync.s,v 1.1 1996/09/25 18:40:38 torek Exp
 */

/*
 * Posix 1j (Draft 5) spinlock implementation for sparc processors.
 * UNTESTED
 */

#include "DEFS.h"

#define	EBUSY		16		/* From errno.h */
/*
 * N.B.: we depend on the fact that ldstub gets 0xff for locked, 0x00 for
 * unlocked, and (0xff & EBUSY) == EBUSY, while (0 & EBUSY) == 0.
 */

/*
 * int spin_init(spinlock_t *m)
 *
 * Returns:	0 = 	Success
 * 
 * Initialize a spin lock data type.
 * This routine never fails.
 */
ENTRY(spin_init)
	st	%g0, [%o0]		/* *m = 0; */
	retl				/* and return success */
	 clr	%o0

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
	ldstub	[%o0], %o1		/* lock it */
	retl				/* return EBUSY if already locked */
	 and	%o1, EBUSY, %o0

/*
 * int spin_lock(spinlock_t *m)
 *
 * Returns:	0 = 	Success
 * 
 * Spin until a spinlock can be aquired.
 * This routine never fails.
 */
ENTRY(spin_lock)
1:
	ldstub	[%o0], %o1		/* lock it */
	tst	%o1			/* loop if already locked */
	bne	1b
	 nop
	retl				/* return success */
	 clr	%o0

/*
 * int spin_trylock(spinlock_t *m)
 *
 * Returns:	0  	Success
 *		EBUSY	Another locker currently holds the spinlock
 *
 * Try to lock a spinlock, returning 0 on success or EBUSY if another
 * caller currently holds the spinlock.
 *
 * (this is now identical to spin_destroy -- combine them?)
 */
ENTRY(spin_trylock)
	ldstub	[%o0], %o1		/* lock it */
	retl				/* return EBUSY if already locked */
	 and	%o1, EBUSY, %o0

/*
 * int spin_unlock(spinlock_t *m)
 *
 * Returns:	0 = 	Success
 * 
 * Unlock a currently locked spinlock.
 * This routine never fails.
 */
ENTRY(spin_unlock)
	st	%g0, [%o0]		/* we assume we own it => clr is safe */
	retl
	 clr	%o0

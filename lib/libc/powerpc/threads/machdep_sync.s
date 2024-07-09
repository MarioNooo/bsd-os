/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996, 2001 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI machdep_sync.s,v 1.2 2001/10/03 17:29:54 polk Exp
 */

	.file "machdep_sync.s"

/*
 * XXX This API doesn't appear to be used by anything right now.
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
	.long 0

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
	.long 0

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
	.long 0

/*
 * int spin_unlock(spinlock_t *m)
 *
 * Returns:	0 = 	Success
 * 
 * Unlock a currently locked spinlock.
 * This routine never fails.
 */
ENTRY(spin_unlock)
	.long 0

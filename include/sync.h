/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI sync.h,v 1.1 1996/06/03 14:59:26 mdickson Exp 
 */

/*
 *	Posix 1j (Draft 5) spin locking primitives definitions
 */

#ifndef	_SYNC_H_
#define	_SYNC_H_

#define	NCPUS	1		/* XXX */

#include <machine/sync.h>

__BEGIN_DECLS
int spin_init __P((spinlock_t *));
int spin_destroy __P((spinlock_t *));
int spin_lock __P((spinlock_t *));
int spin_trylock __P((spinlock_t *));
int spin_unlock __P((spinlock_t *));
__END_DECLS

#endif /* !_SYNC_H_ */

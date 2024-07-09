/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI aout_nlist_offset.c,v 2.3 2001/10/03 17:29:52 polk Exp
 */

#include <sys/types.h>
#include <errno.h>
#include <a.out.h>

#ifdef OMAGIC

#include "nlist_var.h"

/*
 * Find the offset in the mapped binary file that corresponds to
 * the given address in the memory image, if there is one.
 */
off_t
_aout_nlist_offset(unsigned long address, const void *v, size_t size)
{
	const struct exec *ah = v;

	/* Sanity check.  */
	if (N_DATOFF(*ah) + ah->a_data > size) {
		errno = EFTYPE;
		return (-1);
	}

	/* Handle a.out format kernel images.  */
	if (ah->a_entry >= 0xe0000000)
		address -= ah->a_entry;

	if (address >= N_DATADDR(*ah) + ah->a_data) {
		errno = EINVAL;
		return (-1);
	}
	if (address >= N_DATADDR(*ah))
		return (N_DATOFF(*ah) + (address - N_DATADDR(*ah)));

	if (address >= N_TXTADDR(*ah) + ah->a_text) {
		errno = EINVAL;
		return (-1);
	}
	if (address >= N_TXTADDR(*ah))
		return (N_TXTOFF(*ah) + (address - N_TXTADDR(*ah)));

	errno = EINVAL;
	return (-1);
}

#endif

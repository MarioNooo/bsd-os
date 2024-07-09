/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI aout_nlist_classify.c,v 2.3 2001/10/03 17:29:52 polk Exp
 */

#include <sys/types.h>
#include <a.out.h>

#ifdef OMAGIC

#include "nlist_var.h"

enum _nlist_filetype
_aout_nlist_classify(const void *v, size_t size)
{
	const struct exec *ah = v;

	if (size < sizeof (*ah) || N_BADMAG(*ah))
		return (_unknown_nlist_filetype);
	return (_aout_nlist_filetype);
}

#endif

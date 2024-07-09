/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI aout.c,v 2.3 2001/10/03 17:29:57 polk Exp
 */

#include <sys/types.h>
#include <a.out.h>
#include <errno.h>
#include <stdio.h>

#include "size.h"

#ifdef	OMAGIC

/*
 * Print the size of the segments in an a.out executable.
 * If this is not an a.out executable, return 0.
 * If we correctly process the executable, return 1.
 */
int
_aout_size(const char *name, const void *v, size_t len)
{
	const struct exec *a = v;

	if (len < sizeof (struct exec))
		return (0);

	if (N_BADMAG(*a))
		return (0);

	display(name, a->a_text, a->a_data, a->a_bss);

	return (1);
}
#endif

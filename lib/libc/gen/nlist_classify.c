/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI nlist_classify.c,v 2.3 2001/10/03 17:29:52 polk Exp
 */

#include <sys/param.h>
#include <a.out.h>

#include "nlist_var.h"

static const nlist_classify_fn classify[] = {
#ifdef OMAGIC
	_aout_nlist_classify,
#else
	NULL,
#endif
	_elf32_nlist_classify,
#ifdef NLIST64
	_elf64_nlist_classify,
#else
	NULL,
#endif
};

enum _nlist_filetype
_nlist_classify(const void *v, size_t size)
{
	enum _nlist_filetype r;
	size_t i;

	for (i = 0; i < sizeof (classify) / sizeof (classify[0]); ++i)
		if (classify[i] != NULL &&
		    (r = (*classify[i])(v, size)) != _unknown_nlist_filetype)
			return (r);
	return (_unknown_nlist_filetype);
}

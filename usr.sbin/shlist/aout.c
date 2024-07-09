/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI aout.c,v 2.3 2001/10/03 17:30:00 polk Exp
 */

#include <sys/types.h>
#include <a.out.h>
#include <errno.h>
#include <stdio.h>

#include "shlist.h"

#ifdef	OMAGIC
/*
 * Print the list of shared libraries used by an a.out executable.
 * If this is not an a.out executable, return 0.
 * If we encounter a corrupt a.out executable, return -1 with EFTYPE.
 * If we correctly process the executable, return 1.
 */
int
_aout_shlist(const char *name, void *v, size_t len)
{
	struct exec *a = v;
	struct ldtab *lp;
	char *base = v;
	long segoff;
	long ltaddr;

	if (len < sizeof (struct exec))
		return (0);

	if (N_BADMAG(*a))
		return (0);

	if (len < N_DATOFF(*a) + a->a_data) {
		errno = EFTYPE;
		return (-1);
	}

	if ((ltaddr = md_ltaddr(v)) == 0)
		return (1);

	printf("%s:", name);

	segoff = N_TXTOFF(*a) - N_TXTADDR(*a);
	lp = (struct ldtab *)(base + segoff + ltaddr);
	for (; lp->name; ++lp) {
		printf(" %s", base + segoff + (long)lp->name);
		if (aflag)
			printf("<%#10lx>", (unsigned long)lp->address);
	}

	printf("\n");

	return (1);
}
#endif

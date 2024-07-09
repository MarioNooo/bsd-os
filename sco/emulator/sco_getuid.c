/*
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_getuid.c,v 2.1 1995/02/03 15:14:30 polk Exp
 */

/*
 * Support for uid/gid ops
 */

#include <sys/param.h>
#include <err.h>
#include <stdlib.h>
#include <unistd.h>

#include "emulate.h"
#include "sco.h"

uid_t
sco_getuid()
{

	/* iBCS2 p 3-35 documents EDX oddity */
	*program_edx = geteuid();
	return (getuid());
}

gid_t
sco_getgid()
{

	/* iBCS2 p 3-35 documents EDX oddity */
	*program_edx = getegid();
	return (getgid());
}

/*
 * We cheat and perform the array transformation here instead
 * of in a transformation routine, since the length of the array
 * is not implied by its data.
 */
int
sco_getgroups(n, list)
	int n;
	gid_t *list;
{
	gid_t *bgidlist, i;
	unsigned short *sgidlist = (unsigned short *)list;

	if ((bgidlist = malloc(n * sizeof(int))) == 0)
		err(1, "sco_getgroups");

	if ((n = getgroups(n, bgidlist)) != -1)
		for (i = 0; i < n; ++i)
			sgidlist[i] = bgidlist[i];

	free(bgidlist);

	return (n);
}

int
sco_setgroups(n, list)
	int n;
	const gid_t *list;
{
	gid_t *bgidlist;
	const unsigned short *sgidlist = (const unsigned short *)list;
	int i;

	if ((bgidlist = malloc(n * sizeof(int))) == 0)
		err(1, "sco_setgroups");

	for (i = 0; i < n; ++i)
		bgidlist[i] = sgidlist[i];

	n = setgroups(n, bgidlist);

	free(bgidlist);

	return (n);
}

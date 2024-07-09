/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI fd_realloc.c,v 2.1 1996/06/19 18:57:18 bostic Exp
 */

#include <sys/types.h>
#include <sys/time.h>		/* for pselect/timespec, not yet needed */

#include <stdlib.h>
#include <string.h>

struct fd_set *
__fd_realloc(fdset, old_nfd, new_nfd)
	struct fd_set *fdset;
	int old_nfd, new_nfd;
{
	int oldsize, newsize;

	oldsize = __howmany(old_nfd, NFDBITS) * sizeof(fd_mask);
	newsize = __howmany(new_nfd, NFDBITS) * sizeof(fd_mask);
	if (oldsize < newsize &&
	    (fdset = realloc(fdset, newsize)) != NULL)
		memset((char *)fdset + oldsize, 0, newsize - oldsize);
	return (fdset);
}

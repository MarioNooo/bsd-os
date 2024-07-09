/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 2001 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI readdir_r.c,v 2.2 2001/10/03 17:29:52 polk Exp
 */

#include <sys/param.h>
#include <dirent.h>
#include <pthread.h>

#include "thread_private.h"

/*
 * Get next entry in a directory -- reentrant version.
 *
 * The caller must supply a "struct dirent *" containing a valid d_name
 * field of the necessary length.  On return, *retval == space if there
 * is a valid entry, or *retval == NULL at end-of-dir.
 */
int
readdir_r(DIR *dirp, struct dirent *space, struct dirent **retval)
{
	struct dirent *dp;

	/*
	 * We use the lock recursion feature, so we can just call
	 * readdir here to do all the dirty work.  We only need to
	 * bother with locking against directory buffer changes when
	 * the __DTF_READALL flag is not already set.  When that flag
	 * is set, the buffer contains the entire directory as read
	 * at the opendir() call, so it never changes.
	 */
	if ((dirp->dd_flags & __DTF_READALL) == 0)
		(void) _thread_qlock_lock(&dirp->dd_threadlock);
	dp = readdir(dirp);
	if (dp != NULL) {
		/* dp points into dirp->dd_buf -- copy to supplied space. */
		*space = *dp;	/* our d_name is an array, not a pointer */
		*retval = space;
	} else
		*retval = NULL;
	if ((dirp->dd_flags & __DTF_READALL) == 0)
		(void) _thread_qlock_unlock(&dirp->dd_threadlock);
	return (0);
}

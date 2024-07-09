/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_getrlimit.c,v 1.3 1998/10/06 05:09:40 pjd Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_sys_getrlimit(resource, rlp)
	int resource;
	struct rlimit *rlp;
{
	int ret;

	TR("_thread_sys_getrlimit", resource, rlp);

	_thread_kern_enter();

	if ((ret = _syscall_sys_getrlimit(resource, rlp)) == 0) {
		if (resource == RLIMIT_NOFILE)
			_thread_aio_dtablesize = rlp->rlim_cur;
	}

	_thread_kern_exit();
	return (ret);
}

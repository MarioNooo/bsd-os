/*-
 * Copyright (c) 1999 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_setprlimit.c,v 1.1 1999/10/06 22:52:08 jch Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_sys_setprlimit(idtype, id, resource, rlp)
	int idtype;
	int id;
	int resource;
	const struct rlimit *rlp;
{
	int ret;

	TR("_thread_sys_setprlimit", resource, rlp);

	_thread_kern_enter();

	if ((ret = _syscall_sys_setprlimit(idtype, id, resource, rlp)) == 0) {
		if (idtype == P_PID && (id == 0 || id == getpid())
		    && resource == RLIMIT_NOFILE)
			_thread_aio_dtablesize = rlp->rlim_cur;
	}

	_thread_kern_exit();
	return (0);
}

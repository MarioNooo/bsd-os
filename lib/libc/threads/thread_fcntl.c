/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_fcntl.c,v 1.5 2000/08/24 15:54:29 jch Exp 
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_sys_fcntl(fd, cmd, arg)
	int fd;
	int cmd;
	int arg;
{
	thread_aio_desc_t *dp;
	int ret;

	TR("_thread_sys_fcntl", fd, cmd);

	_thread_kern_enter();

	if (fd < 0 || fd >= _thread_aio_table_size ||
	    (dp = _thread_aio_table[fd]) == NULL || dp->aiofd_object == NULL) {
		errno = EBADF;
		ret = -1;
		goto Return;
	}

	switch (cmd) {
	case F_DUPFD:
		if ((ret = _syscall_sys_fcntl(fd, cmd, arg)) >= 0) {
			if ((errno = _thread_aio_entry_dup(fd, ret)) != 0) {
				_syscall_sys_close(ret);
				ret = -1;
			}
		}
		break;

	case F_SETFD:
		ret = _syscall_sys_fcntl(fd, cmd, arg | FD_NONBLOCK);
		if (ret != -1)
			dp->aiofd_fdflags = arg;
		break;

	case F_GETFD:
		ret = dp->aiofd_fdflags;
		break;

	case F_GETFL:
		/* Refresh our copy of the file object flags */
		ret = _syscall_sys_fcntl(fd, cmd, arg);
		if (ret != -1)
			dp->aiofd_object->aiofo_oflags = ret;
		break;

	case F_SETFL:
		ret = _syscall_sys_fcntl(fd, cmd, arg);
		if (ret != -1)
			dp->aiofd_object->aiofo_oflags = arg;
		break;

	case F_SETLKW:
		_thread_testcancel();
		/* Fall through */

	default:
		ret = _syscall_sys_fcntl(fd, cmd, arg);
		break;
	}

 Return:
	_thread_kern_exit();
	return (ret);
}

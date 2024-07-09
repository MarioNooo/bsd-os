/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_writev.c,v 1.4 2000/08/24 15:54:31 jch Exp
 */

#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/uio.h>

#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

ssize_t
_thread_sys_writev(fd, iov, iovcnt)
	int fd;
	const struct iovec * iov;
	int iovcnt;
{
	int	ret;

	TR("_thread_sys_writev", fd, iov);

	errno = 0;
	_thread_kern_enter();

	THREAD_AIO(fd, FD_WRITE, ret,
	    _syscall_sys_writev(fd, iov, iovcnt),
	    break);

	_thread_kern_exit();
	return (ret);
}

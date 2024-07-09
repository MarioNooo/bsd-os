/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_connect.c,v 1.8 2000/09/08 20:33:06 jch Exp
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_sys_connect(fd, name, namelen)
	int fd;
	const struct sockaddr * name;
	int namelen;
{
	size_t size;
	int error;
	int ret;

 	TR("_thread_sys_connect", fd, name);

	errno = 0;

	_thread_kern_enter();

	if (fd < 0 || fd >= _thread_aio_table_size) {
		_thread_kern_exit();
		errno = EBADF;
		return (-1);
	}

	if ((ret = _syscall_sys_connect(fd, name, namelen)) == 0)
		goto Done;

	if (_thread_aio_table[fd]->aiofd_fdflags & FD_NONBLOCK)
		goto Done;

	if (errno != EINPROGRESS)
		goto Done;

	if ((errno = _thread_aio_suspend(fd, FD_WRITE))) {
		ret = -1;
		goto Done;
	}

	size = sizeof(error);
	if ((ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &size)) == 0 &&
	    ret == 0 && error != 0)
		ret = -1;
	errno = error;

 Done:
	_thread_kern_exit();
	return (ret);
}

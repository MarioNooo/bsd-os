/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_sendto.c,v 1.4 2000/08/24 15:54:31 jch Exp
 */
#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

ssize_t
_thread_sys_sendto(fd, msg, len, flags, to, to_len)
	int fd;
	const void *msg;
	size_t len;
	int flags;
	const struct sockaddr * to;
	int to_len;
{
	int	ret;

	TR("_thread_sys_sendto", fd, msg);

	errno = 0;
	_thread_kern_enter();

	THREAD_AIO(fd, FD_WRITE, ret, 
	    _syscall_sys_sendto(fd, msg, len, flags, to, to_len), 
	    break);

	_thread_kern_exit();
	return (ret);
}

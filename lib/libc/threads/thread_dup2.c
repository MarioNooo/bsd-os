/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_dup2.c,v 1.3 2000/08/24 15:54:29 jch Exp 
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_sys_dup2(fd, newfd)
	int fd;
	int newfd;
{

	TR("_thread_sys_dup2", fd, newfd);

	if (_syscall_sys_dup2(fd, newfd) == -1)
		return (-1);

	_thread_kern_enter();

	if (fd != newfd && (errno = _thread_aio_entry_dup(fd, newfd)) != 0) {
		_syscall_sys_close(newfd);
		_thread_kern_exit();
		return (-1);
	}

	_thread_kern_exit();
	return (0);
}

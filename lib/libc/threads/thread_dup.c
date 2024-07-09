/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_dup.c,v 1.2 1997/07/25 20:33:31 pjd Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_sys_dup(fd)
	int fd;
{
	int newfd;

	TR("_thread_sys_dup", fd, 0);

	if ((newfd = _syscall_sys_dup(fd)) == -1)
		return (newfd);

	_thread_kern_enter();
	if ((errno = _thread_aio_entry_dup(fd, newfd)) != 0) {
		_syscall_sys_close(newfd);
		_thread_kern_exit();
		return (-1);
	}

	_thread_kern_exit();
	return (newfd);
}

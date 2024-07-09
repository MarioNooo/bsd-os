/*
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_open.c,v 1.4 2000/08/24 15:54:30 jch Exp 
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_sys_open(path, flags, mode)
	const char *path;
	int flags;
	int mode;
{
	int error, fd;

	TR("_thread_sys_open", path, flags);

	/* Open the file, forcing it to use non-blocking I/O operations: */
	if ((fd = _syscall_sys_open(path, flags | O_NONBLOCK, mode)) == -1)
		return (fd);

	_thread_kern_enter();

	if ((flags & O_NONBLOCK) == 0) {
		flags = _syscall_sys_fcntl(fd, F_GETFL, 0);
		_syscall_sys_fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
	}

	/* Initialise the file descriptor table entry */
	if ((error = _thread_aio_entry_init(fd)) != 0) {
		_syscall_sys_close(fd);
		fd = -1;
		errno = error;
	}

	_thread_kern_exit();
	return (fd);
}

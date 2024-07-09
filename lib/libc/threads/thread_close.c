/*
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_close.c,v 1.5 2003/04/07 13:33:40 giff Exp
 */

#include <sys/types.h>
#include <sys/fcntl.h>

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_sys_close(fd)
	int fd;
{
	int ret;

	TR("_thread_sys_close", fd, 0);

	_thread_kern_enter();
	_thread_testcancel();

	if (fd < 0 || fd >= _thread_aio_table_size) {
		_thread_kern_exit();
		errno = EBADF;
		return (-1);
	}

	if (_thread_aio_table[fd] != NULL &&
	    _thread_aio_table[fd]->aiofd_object != NULL)
		_thread_aio_entry_destroy(fd);
	_thread_aio_table[fd] = NULL;
	ret = _syscall_sys_close(fd);

	_thread_kern_exit();
	return (ret);
}

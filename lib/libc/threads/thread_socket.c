/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_socket.c,v 1.3 2000/08/24 15:54:31 jch Exp 
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
_thread_sys_socket(af, type, protocol)
	int af;
	int type;
	int protocol;
{
	int fd;

	TR("_thread_sys_socket", af, type);

	if ((fd = _syscall_sys_socket(af, type, protocol)) < 0)
		return (fd);

	_thread_kern_enter();

	/* Initialize the table entry */
	if ((errno = _thread_aio_entry_init(fd)) != 0) {
		_syscall_sys_close(fd);
		fd = -1;
	}

	_thread_kern_exit();
	return (fd);
}

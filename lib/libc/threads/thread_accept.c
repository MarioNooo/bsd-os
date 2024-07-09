/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_accept.c,v 1.4 2000/08/24 15:54:28 jch Exp
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
_thread_sys_accept(fd, name, namelen)
	int 	fd;
	struct 	sockaddr * name;
	int *	namelen;
{
	int newfd;

	TR("_thread_sys_accept", fd, name);

	errno = 0;
	_thread_kern_enter();

	THREAD_AIO(fd, FD_READ, newfd,
	    _syscall_sys_accept(fd, name, namelen),
	    _thread_kern_exit(); return (-1));

	/* Initialise the file descriptor table entry */
	if ((errno = _thread_aio_entry_init(newfd)) != 0) {
		_syscall_sys_close(newfd);
		newfd = -1;
	}

	_thread_kern_exit();
	return (newfd);
}

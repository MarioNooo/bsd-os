/*
 * Copyright (c) 1999 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_fsync.c,v 1.1 1999/10/04 18:16:11 jch Exp
 */

#include <sys/types.h>

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_sys_fsync(fd)
	int fd;
{
	int ret;

	TR("_thread_sys_fsync", fd, 0);

	_thread_kern_enter();
	_thread_testcancel();
	ret = _syscall_sys_fsync(fd);

	_thread_kern_exit();
	return (ret);
}

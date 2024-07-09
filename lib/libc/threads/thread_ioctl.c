/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 2001 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_ioctl.c,v 1.2 2001/10/03 17:29:55 polk Exp 
 */

#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_sys_ioctl(fd, cmd, arg)
	int fd;
	unsigned long cmd;
	char *arg;
{
	thread_aio_desc_t *dp;
	int ret;

	TR("_thread_sys_ioctl", fd, cmd);

	_thread_kern_enter();

	if (fd < 0 || fd >= _thread_aio_table_size ||
	    (dp = _thread_aio_table[fd]) == NULL || dp->aiofd_object == NULL) {
		errno = EBADF;
		ret = -1;
		goto Return;
	}

	switch (cmd) {
	case FIONBIO:
		if ((ret = _syscall_sys_ioctl(fd, cmd, arg)) >= 0) {
			if (*arg != 0)
				dp->aiofd_object->aiofo_oflags |= FNONBLOCK;
			else
				dp->aiofd_object->aiofo_oflags &= ~FNONBLOCK;
		}
		break;

	case FIOASYNC:
		if ((ret = _syscall_sys_ioctl(fd, cmd, arg)) >= 0) {
			if (*arg != 0)
				dp->aiofd_object->aiofo_oflags |= FASYNC;
			else
				dp->aiofd_object->aiofo_oflags &= ~FASYNC;
		}
		break;

	default:
		ret = _syscall_sys_ioctl(fd, cmd, arg);
		break;
	}

 Return:
	_thread_kern_exit();
	return (ret);
}

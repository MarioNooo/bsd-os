/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_pipe.c,v 1.4 2000/08/24 15:54:30 jch Exp 
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_sys_pipe(filedes)
	int filedes[2];
{
	int rc;

	TR("_thread_sys_pipe", filedes, 0);

	if (_syscall_sys_pipe(filedes) != 0) 
		return (-1);

	_thread_kern_enter();

	/* Initialize the table entry */
	if ((errno = _thread_aio_entry_init(filedes[0])) != 0) {
	Close:
		_syscall_sys_close(filedes[0]);
		_syscall_sys_close(filedes[1]);
		rc = -1;
	} else if ((errno = _thread_aio_entry_init(filedes[1])) != 0) {
		_thread_aio_entry_destroy(filedes[0]);
		goto Close;
		rc = -1;
	} else
		rc = 0;

	_thread_kern_exit();
	return (rc);
}

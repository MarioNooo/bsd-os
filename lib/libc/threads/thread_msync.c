/*
 * Copyright (c) 1999 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_msync.c,v 1.1 1999/10/04 18:16:11 jch Exp
 */

#include <sys/types.h>
#include <sys/mman.h>

#include <pthread.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_sys_msync(void *addr, int len, int flags)
{
	int ret;

	TR("_thread_sys_msync", addr, flags);

	_thread_kern_enter();
	_thread_testcancel();
	ret = _syscall_sys_msync(addr, len, flags);

	_thread_kern_exit();
	return (ret);
}

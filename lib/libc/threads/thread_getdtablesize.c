/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_getdtablesize.c,v 1.1 1996/06/03 15:24:46 mdickson Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"

/*
 * _thread_sys_getdtablesize:
 *
 * Description:
 * 	Return the current file table size.
 */

int
_thread_sys_getdtablesize()
{
	return (_thread_aio_dtablesize);
}

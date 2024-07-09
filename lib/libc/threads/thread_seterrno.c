/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_seterrno.c,v 1.2 1997/07/25 20:35:41 pjd Exp
 */

#include <errno.h>
#include <pthread.h>

#include "thread_private.h"
#include "thread_trace.h"

#undef _thread_seterrno
void
_thread_seterrno(threadp, erc)
	struct pthread * threadp;
	int erc;
{

	TR("_thread_seterrno", threadp, erc);

	if (threadp == _thread_selfp())
		errno = erc;
	else
		threadp->error = erc;
}

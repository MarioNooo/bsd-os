/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI flockfile.c,v 2.9 2001/10/03 17:29:54 polk Exp 
 */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "local.h"
#include "thread_private.h"

void
__sflfinit(fp)
	FILE *fp;
{

	(void) _thread_qlock_init(&fp->_flfp);
}

void 
__sflffree(fp)
	FILE *fp;
{

	_thread_qlock_free(&fp->_flfp);
}

void
flockfile(fp)
	FILE *fp;
{

	if (THREAD_SAFE() && fp->_flags != 0 && (fp->_flags & __SLOC) == 0)
		_thread_qlock_lock(&fp->_flfp);
}

int
ftrylockfile(fp)
	FILE *fp;
{

	if (THREAD_SAFE() && fp->_flags != 0 && (fp->_flags & __SLOC) == 0)
		return (_thread_qlock_try(&fp->_flfp));
	return (0);
}

void
funlockfile(fp)
	FILE *fp;
{

	if (THREAD_SAFE() && fp->_flags != 0 && (fp->_flags & __SLOC) == 0)
		(void) _thread_qlock_unlock(&fp->_flfp);
}

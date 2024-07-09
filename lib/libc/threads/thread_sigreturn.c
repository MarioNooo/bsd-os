/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 * 
 * BSDI thread_sigreturn.c,v 1.3 1997/09/10 17:37:18 pjd Exp
 */

#include <sys/types.h>

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>

#include "thread_private.h"
#include "thread_trace.h"

/*
 * _thread_sys_sigreturn:
 *
 * Description:
 * 	Return specifying an alternate register context.
 */

int
_thread_sys_sigreturn (scp)
	struct sigcontext * scp;
{

	TR("_thread_sys_sigreturn", scp, 0);

	kill(getpid(), SIGABRT);
	/* NOTREACHED */
	return (-1);
}

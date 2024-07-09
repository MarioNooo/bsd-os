/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_sigaltstack.c,v 1.3 1997/09/10 17:37:03 pjd Exp
 */

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

/*
 * _thread_sys_sigaltstack:
 *
 * Description:
 * 	Install an alternate stack to use for signal delivery.
 */

int
_thread_sys_sigaltstack(ss, oss)
	const struct sigaltstack *ss;
	struct sigaltstack *oss;
{

	TR("_thread_sys_sigaltstack", ss, oss);

	kill(getpid(), SIGABRT);
	/* NOTREACHED */
	return (-1);
}

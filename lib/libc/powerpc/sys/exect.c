/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI exect.c,v 1.1 1995/12/18 21:50:36 donn Exp
 */

/*
 * Exec a new image and stop immediately with a trace trap.
 * Since we give a trace trap immediately after exec when tracing anyway,
 * we just make sure that we're tracing and do a normal exec.
 */

#include <sys/types.h>
#include <sys/ptrace.h>
#include <unistd.h>

int
exect(path, argv, envp)
	const char *path;
	char * const *argv;
	char * const *envp;
{

	ptrace(PT_TRACE_ME, 0, NULL, 0);
	return (execve(path, argv, envp));
}

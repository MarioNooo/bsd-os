/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI setlogin.c,v 1.1 1995/12/18 21:50:42 donn Exp
 */

/*
 * Caching version of setlogin().
 */

#include <sys/syscall.h>
#include <unistd.h>

extern int _logname_valid;

int
setlogin(const char *s)
{

	if (syscall(SYS_setlogin, s) == -1)
		return (-1);
	_logname_valid = 0;
	return (0);
}

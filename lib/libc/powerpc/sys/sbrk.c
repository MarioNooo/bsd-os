/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sbrk.c,v 1.1 1995/12/18 21:50:40 donn Exp
 */

/*
 * Incremental heap allocation.
 */

#include <sys/syscall.h>
#include <errno.h>
#include <unistd.h>

extern char _end;

char *_minbrk = &_end;
char *_curbrk = &_end;

char *
sbrk(int incr)
{
	char *new;

	if (syscall(SYS_break, _curbrk + incr) == -1)
		return ((char *)-1);
	new = _curbrk;
	_curbrk += incr;
	return (new);
}

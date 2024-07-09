/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI brk.c,v 1.3 2001/10/03 17:29:53 polk Exp
 */

/*
 * Heap allocation.
 */

#include <sys/syscall.h>
#include <errno.h>
#include <unistd.h>

extern char *_minbrk;
extern char *_curbrk;

char *
brk(const char *new)
{

	if (new < _minbrk)
		new = _minbrk;
	if (syscall(SYS_break, new) == -1)
		return ((char *)-1);
	_curbrk = (char *)new;
	return (NULL);
}

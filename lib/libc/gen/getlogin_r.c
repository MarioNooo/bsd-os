/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 2001 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI getlogin_r.c,v 2.2 2001/10/03 17:29:52 polk Exp
 */

#include <sys/param.h>
#include <errno.h>

int
getlogin_r(char *buf, size_t size)
{
	int e, ret;

	/* our "size" argument includes space for the '\0' */
	if (size < 1)
		return (ERANGE);
	/* would be nice if the _getlogin syscall returned an errno... */
	e = errno;
	if (_getlogin(buf, size - 1) < 0) {
		ret = errno;
		errno = e;
		return (ret);
	}

	/*
	 * The system call does not necessarily terminate the buffer.
	 * We could see if it did (memchr(buf, '\0', size) != NULL)
	 * but we can just smoosh in a '\0' at the end, which is simpler
	 * if sometimes redundant.
	 */
	buf[size - 1] = '\0';
	return (0);
}

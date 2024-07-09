/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ftok.c,v 2.1 1996/07/09 15:04:16 mdickson Exp
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>

/* from stdipc(3C) description in SVr4 API p 866 */

key_t
ftok(name, cookie)
	const char *name;
	int cookie;
{
	key_t k;
	struct stat s;

	if (stat(name, &s) == -1)
		return ((key_t)-1);

	/*
	 * Linked files are supposed to get the same key
	 * if they have the same cookie.
	 * We do the obvious thing and munge dev/inode bits.
	 */
	k = ((s.st_dev & 0xffff) << 16) + (s.st_dev >> 16 & 0xffff);
	k += s.st_ino;
	k += (cookie & 0xff) << 24;

	return (k);
}

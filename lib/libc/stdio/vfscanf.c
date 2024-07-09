/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI vfscanf.c,v 2.2 1996/06/03 15:24:23 mdickson Exp 
 */

#include <stdio.h>

int
vfscanf(fp, fmt, ap)
	register FILE *fp;
	const char *fmt;
	_BSD_VA_LIST_ ap;
{
	int ret;

	flockfile(fp);
	ret = __svfscanf(fp, fmt, ap);
	funlockfile(fp);

	return (ret);
}

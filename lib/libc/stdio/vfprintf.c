/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI vfprintf.c,v 2.4 1996/06/03 15:24:23 mdickson Exp 
 */

#include <stdio.h>

int
vfprintf(fp, fmt, ap)
	FILE *fp;
	const char *fmt;
	_BSD_VA_LIST_ ap;
{
	int ret;

	flockfile(fp);
	ret = __svfprintf(fp, fmt, ap);
	funlockfile(fp);
	return (ret);
}

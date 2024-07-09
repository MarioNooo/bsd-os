/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI sockatmark.c,v 2.1 1997/09/04 23:00:56 karels Exp
 */

#include <sys/ioctl.h>

int
sockatmark(s)
	int s;
{
	int atmark;

	if (ioctl(s, SIOCATMARK, &atmark) == -1)
		return (-1);
	return (atmark);
}

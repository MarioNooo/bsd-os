/*
 * Copyright (c) 1995 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI usleep.c,v 2.6 1996/05/22 14:59:48 jch Exp
 */

#include <errno.h>
#include <time.h>
#include <unistd.h>

/*
 * usleep --
 *	Pause N microseconds or until a signal arrives, returning nothing.
 */
void
usleep(usecs)
	unsigned int usecs;
{
	struct timeval t;

	if (usecs != 0) {
		t.tv_sec = usecs / 1000000;
		t.tv_usec = usecs % 1000000;

#define	MSG \
	"usleep: specified number of microseconds negative or too large.\n"
		if (select(0, NULL, NULL, NULL, &t) == -1 && errno == EINVAL)
			(void)write(STDERR_FILENO, MSG, sizeof(MSG) - 1);
	}
}

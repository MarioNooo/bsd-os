/*
 * Copyright (c) 1995 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sleep.c,v 2.5 1996/05/15 17:42:28 jch Exp
 */

#include <sys/time.h>

#include <errno.h>
#include <unistd.h>

/*
 * sleep --
 *	Pause N seconds or until a signal arrives, returning 0 or the
 *	remaining sleep time if interrupted.
 */
u_int
sleep(secs)
	u_int secs;
{
	struct timeval nt, ot;
	long diff;
	int rc;

	if (secs == 0)
		return (0);

	/*
	 * XXX
	 * Sleep has no error return, so there's not much we can do.
	 */
	if (gettimeofday(&ot, NULL))
		return (secs);

	nt.tv_sec = secs;
	nt.tv_usec = 0;
	if ((rc = select(0, NULL, NULL, NULL, &nt)) == 0)
		return (0);

#define	MSG	"sleep: specified number of seconds negative or too large.\n"
	if (rc < 0 && errno == EINVAL)
		(void)write(STDERR_FILENO, MSG, sizeof(MSG) - 1);
	/*
	 * XXX
	 * If the date has changed, sleep will return the wrong answer.
	 */
	if (gettimeofday(&nt, NULL))
		return (1);

	diff = secs - (nt.tv_sec - ot.tv_sec);
	if (diff <= 0)
		diff = 1;
	return (diff);
}

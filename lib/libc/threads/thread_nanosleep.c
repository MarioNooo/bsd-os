/*
 * Copyright (c) 1995, 2000 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI thread_nanosleep.c,v 1.2 2000/08/24 15:54:30 jch Exp
 */

#include <sys/time.h>

#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "thread_private.h"

int
_thread_sys_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	struct timespec nt, ot;
	long diff;
	int rc;

	if (rqtp == NULL || timespecinvalid(rqtp)) {
		errno = EINVAL;
		return (-1);
	}
	if (rqtp->tv_sec ==  0 && rqtp->tv_nsec == 0) {
		if (rmtp != NULL)
			*rmtp = *rqtp;
		return (0);
	}

	if (rmtp != NULL) {
		if (clock_gettime(CLOCK_REALTIME, rmtp)) {
			*rmtp = *rqtp;
			errno = EINVAL;
			return (-1);
		}
		timespecadd(rmtp, rqtp);
	}

	if ((rc = pselect(0, NULL, NULL, NULL, rqtp, NULL)) == 0) {
		if (rmtp != NULL) {
			rmtp->tv_sec = 0;
			rmtp->tv_nsec = 0;
		}
		return (0);
	}

	/*
	 * XXX
	 * If the date has changed, nanosleep will return the wrong answer.
	 */
	if (rmtp != NULL) {
		if (clock_gettime(CLOCK_REALTIME, &nt)) {
			rmtp->tv_sec = 0;
			rmtp->tv_nsec = 0;
		} else {
			timespecsub(rmtp, &nt);
			if (rmtp->tv_sec < 0 ||
			    rmtp->tv_sec ==  0 && rmtp->tv_nsec <= 0) {
				rmtp->tv_sec = 0;
				rmtp->tv_nsec = 0;
			}
		}
	}
	return (rc);
}

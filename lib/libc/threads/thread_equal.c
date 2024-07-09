/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_equal.c,v 1.1 1996/06/03 15:24:45 mdickson Exp
 */

#include <pthread.h>

int
pthread_equal(t1, t2)
	pthread_t t1;
	pthread_t t2;
{
	/* Compare the two thread pointers: */
	return (t1 == t2);
}

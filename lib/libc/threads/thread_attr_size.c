/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_attr_size.c,v 1.3 1997/09/10 17:35:50 pjd Exp
 */

#include <errno.h>
#include <pthread.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_attr_getstacksize(attr, stacksize)
	pthread_attr_t * attr;
	size_t * stacksize;
{

	TR("pthread_attr_getstacksize", attr, stacksize);

	*stacksize = attr->stacksize;
	return (0);
}

int
pthread_attr_setstacksize(attr, stacksize)
	pthread_attr_t * attr;
	size_t stacksize;
{

	TR("pthread_attr_setstacksize", attr, stacksize);

	if (stacksize < PTHREAD_STACK_MIN)
		return (EINVAL);

	attr->stacksize = stacksize;
	return (0);
}

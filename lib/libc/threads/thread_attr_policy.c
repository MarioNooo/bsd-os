/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_attr_policy.c,v 1.5 2000/08/24 15:54:29 jch Exp
 */

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_attr_getschedpolicy(attr, policy)
	pthread_attr_t * attr;
	int * policy;
{

	TR("pthread_attr_getschedpolicy", attr, policy);

	if ((attr == NULL) || (policy == NULL))
		return (EINVAL);

	*policy = attr->schedpolicy;
	return (0);
}

int
pthread_attr_setschedpolicy(attr, policy)
	pthread_attr_t * attr;
	int policy;
{

	TR("pthread_attr_setschedpolicy", attr, policy);

	if (attr == NULL)
		return (EINVAL);

	switch (policy) {
	case SCHED_FIFO:
	case SCHED_RR:
	case SCHED_OTHER:
		break;
	default:
		return (EINVAL);
	}

	attr->schedpolicy = policy;
	return (0);
}

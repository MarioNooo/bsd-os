/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_attr_param.c,v 1.3 1997/09/10 17:35:41 pjd Exp
 */

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_attr_getschedparam(attr, param)
	pthread_attr_t * attr;
	struct sched_param * param;
{

	TR("pthread_attr_getschedparam", attr, param);

	if ((attr == NULL) || (param == NULL))
		return (EINVAL);

	*param = attr->schedparam;
	return (0);
}

int
pthread_attr_setschedparam(attr, param)
	pthread_attr_t * attr;
	const struct sched_param * param;
{

	TR("pthread_attr_setschedparam", attr, param);

	if ((attr == NULL) || (param == NULL))
		return (EINVAL);

	attr->schedparam = *param;
	return (0);
}

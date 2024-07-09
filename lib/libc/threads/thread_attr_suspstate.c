/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_attr_suspstate.c,v 1.3 1997/09/10 17:35:54 pjd Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stddef.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_attr_getsuspendstate_np(attr, suspendstate)
	pthread_attr_t *attr;
	int *suspendstate;
{

	TR("pthread_attr_getsuspendstate_np", attr, suspendstate);

	if ((attr == NULL) || (suspendstate == NULL))
		return (EINVAL);

	*suspendstate = (attr->flags & PTHREAD_CREATE_SUSPENDED);
	return (0);
}

int
pthread_attr_setsuspendstate_np(attr, suspendstate)
	pthread_attr_t * attr;
	int suspendstate;
{

	TR("pthread_attr_setsuspendstate_np", attr, suspendstate);

	if (attr == NULL)
		return (EINVAL);

	/* Clear current setting */
	attr->flags &= ~(PTHREAD_CREATE_SUSPENDED);	
	if (suspendstate == PTHREAD_CREATE_SUSPENDED)
		attr->flags |= suspendstate;	/* Set new ones */
	return (0);
}

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_attr_detachstate.c,v 1.2 1997/07/25 20:32:24 pjd Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stddef.h>

#include "thread_private.h"
#include "thread_trace.h"

#define	DETACH_MASK	(PTHREAD_CREATE_DETACHED | PTHREAD_CREATE_JOINABLE)

int
pthread_attr_getdetachstate(attr, detachstate)
	pthread_attr_t * attr;
	int * detachstate;
{

	TR("pthread_attr_getdetachstate", attr, detachstate);

	if ((attr == NULL) || (detachstate == NULL))
		return (EINVAL);

	*detachstate = (attr->flags & DETACH_MASK);
	return (0);
}

int
pthread_attr_setdetachstate(attr, detachstate)
	pthread_attr_t * attr;
	int detachstate;
{

	TR("pthread_attr_setdetachstate", attr, detachstate);

	if ((attr == NULL) ||
	    ((detachstate != PTHREAD_CREATE_DETACHED) &&
	     (detachstate != PTHREAD_CREATE_JOINABLE)))
		return (EINVAL);

	attr->flags &= ~(DETACH_MASK);	/* Clear current setting */
	attr->flags |= detachstate;	/* Set new ones */
	return (0);
}

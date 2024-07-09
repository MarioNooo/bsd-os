/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_attr_inheritsched.c,v 1.3 1997/09/10 17:35:31 pjd Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stddef.h>

#include "thread_private.h"
#include "thread_trace.h"

#define	INHERIT_MASK	(PTHREAD_INHERIT_SCHED | PTHREAD_EXPLICIT_SCHED)

int
pthread_attr_getinheritsched(attr, inheritsched)
	pthread_attr_t * attr;
	int * inheritsched;
{

	TR("pthread_attr_getinheritsched", attr, inheritsched);

	if ((attr == NULL) || (inheritsched == NULL))
		return (EINVAL);

	*inheritsched = (attr->flags & INHERIT_MASK);
	return (0);
}

int
pthread_attr_setinheritsched(attr, inheritsched)
	pthread_attr_t * attr;
	int inheritsched;
{

	TR("pthread_attr_setinheritsched", attr, inheritsched);

	if ((attr == NULL) ||
	    ((inheritsched != PTHREAD_INHERIT_SCHED) &&
	     (inheritsched != PTHREAD_EXPLICIT_SCHED)))
		return (EINVAL);

	attr->flags &= ~(INHERIT_MASK);	/* Clear current setting */
	attr->flags |= inheritsched;	/* Set new ones */
	return (0);
}

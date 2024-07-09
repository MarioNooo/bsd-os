/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 * 
 * BSDI thread_attr_scope.c,v 1.2 1997/07/25 20:32:45 pjd Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stddef.h>

#include "thread_private.h"
#include "thread_trace.h"

#define	SCOPE_MASK	(PTHREAD_SCOPE_PROCESS | PTHREAD_SCOPE_SYSTEM)

int
pthread_attr_getscope(attr, scope)
	pthread_attr_t * attr;
	int * scope;
{

	TR("pthread_attr_getscope", attr, scope);

	if ((attr == NULL) || (scope == NULL))
		return (EINVAL);

	*scope = (attr->flags & SCOPE_MASK);
	return (0);
}

int
pthread_attr_setscope(attr, scope)
	pthread_attr_t * attr;
	int scope;
{

	TR("pthread_attr_setscope", attr, scope);

	if ((attr == NULL) ||
	    ((scope != PTHREAD_SCOPE_PROCESS) &&
	     (scope != PTHREAD_SCOPE_SYSTEM)))
		return (EINVAL);

	attr->flags &= ~(SCOPE_MASK);	/* Clear current setting */
	attr->flags |= scope;		/* Set new ones */
	return (0);
}

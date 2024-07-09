/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_attr_addr.c,v 1.3 1998/06/02 22:56:15 pjd Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <string.h>

#include "thread_private.h"
#include "thread_trace.h"

int
pthread_attr_getstackaddr(attr, stackaddr)
	pthread_attr_t * attr;
	void ** stackaddr;
{

	TR("pthread_attr_getstackaddr", attr, stackaddr);

	if (attr == NULL || stackaddr == NULL)
		return (EINVAL);

	*stackaddr = attr->stackaddr;
	return (0);
}

int
pthread_attr_setstackaddr(attr, stackaddr)
	pthread_attr_t * attr;
	void * stackaddr;
{

	TR("pthread_attr_setstackaddr", attr, stackaddr);

	if (attr == NULL)
		return (EINVAL);

	attr->stackaddr = stackaddr;
	return (0);
}

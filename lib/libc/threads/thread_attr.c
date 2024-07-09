/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_attr.c,v 1.2 1996/12/11 19:34:49 donn Exp
 */

#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <string.h>

#include "thread_private.h"

/* Default thread attributes: */
extern pthread_attr_t pthread_attr_default;

int
pthread_attr_init(attr)
	pthread_attr_t * attr;
{
	memcpy(attr, &pthread_attr_default, sizeof(pthread_attr_t));
	return (0);
}

int
pthread_attr_destroy(attr)
	pthread_attr_t * attr;
{
	memset(attr, 0, sizeof(pthread_attr_t));
	return (0);
}
